/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */


#include "include/bluetooth_multi_block.h"
#include <math.h>


/* constructor */
bluetooth_multi_block::bluetooth_multi_block(double sample_rate, double center_freq, double squelch_threshold)
{
	d_cumulative_count = 0;
	d_sample_rate = sample_rate;
	d_center_freq = center_freq;
	set_channels();
	
	int slots = 1;
	d_samples_per_symbol = sample_rate / SYMBOL_RATE;
	
	d_samples_per_slot = (int) SYMBOLS_PER_SLOT * d_samples_per_symbol;
	int samples_required = (int) slots * d_samples_per_slot;

	
	d_squelch_threshold = (double) std::pow(10.0, squelch_threshold/10) * d_samples_per_symbol * 68; 

	
	double gain = 1;
	double cutoff_freq = 500000;
	double transition_width = 300000;
	d_channel_filter = gr::filter::firdes::low_pass(gain, sample_rate, cutoff_freq, transition_width, gr::filter::firdes::WIN_HANN);
	
	samples_required += (d_channel_filter.size() - 1);

	
	d_ddc_decimation_rate = (int) d_samples_per_symbol / 2;
	double channel_samples_per_symbol = d_samples_per_symbol / d_ddc_decimation_rate;

	
	d_demod_gain = channel_samples_per_symbol / M_PI_2;

	
	d_gain_mu = 0.175;
	d_mu = 0.32;
	d_omega_relative_limit = 0.005;
	d_omega = channel_samples_per_symbol;
	d_gain_omega = .25 * d_gain_mu * d_gain_mu;
	d_omega_mid = d_omega;
	d_interp = new gr::filter::mmse_fir_interpolator_ff();
	d_last_sample = 0;
	samples_required += d_ddc_decimation_rate * d_interp->ntaps();

	set_history(samples_required);
}

static inline float
slice(float x)
{
	return x < 0 ? -1.0F : 1.0F;
}


int bluetooth_multi_block::mm_cr(const float *in, int ninput_items, float *out, int noutput_items)
{
	unsigned int ii = 0; 
	int op = 0; 
	unsigned int ni = ninput_items - d_interp->ntaps(); 
	float mm_val;

	while (op < noutput_items && ii < ni)
	{
		
		out[op] = d_interp->interpolate (&in[ii], d_mu);
		mm_val = slice(d_last_sample) * out[op] - slice(out[op]) * d_last_sample;
		d_last_sample = out[op];

		d_omega += d_gain_omega * mm_val;
		d_omega = d_omega_mid + gr::branchless_clip(d_omega-d_omega_mid, d_omega_relative_limit);   
		d_mu += d_omega + d_gain_mu * mm_val;

		ii += (int) floor(d_mu);
		d_mu -= floor(d_mu);
		op++;
		
		if (ii > 32768) printf("%d, %d, %d, %d\n", op, noutput_items, ii, ni);
	}

	
	return op;
}


void bluetooth_multi_block::demod(const gr_complex *in, float *out, int noutput_items)
{
	int i;
	gr_complex product;

	for (i = 1; i < noutput_items; i++)
	{
		gr_complex product = in[i] * conj (in[i-1]);
		out[i] = d_demod_gain * gr::fast_atan2f(imag(product), real(product));
	}
}


void bluetooth_multi_block::slicer(const float *in, char *out, int noutput_items)
{
	int i;

	for (i = 0; i < noutput_items; i++)
		out[i] = (in[i] < 0) ? 0 : 1;
}


int bluetooth_multi_block::channel_symbols(int channel, gr_vector_const_void_star &in, char *out, int ninput_items)
{
	
	int i;
	double pwr = 0; 
	gr_complex *raw_in = (gr_complex *) in[0];
	int last_sq = d_samples_per_symbol * (SYMBOLS_PER_SLOT + 68);
	if (ninput_items < last_sq)
		last_sq = ninput_items;
	for (i = 0; i < last_sq; i++)
		pwr += (raw_in[i].real() * raw_in[i].real() + raw_in[i].imag() * raw_in[i].imag());
	if (pwr < d_squelch_threshold)
		
		return 0;

	
	double ddc_center_freq = channel_freq(channel);
	gr::filter::freq_xlating_fir_filter_ccf::sptr ddc =
		gr::freq_xlating_fir_filter_ccf::make(d_ddc_decimation_rate, d_channel_filter, ddc_center_freq, d_sample_rate);
	int ddc_noutput_items = ddc->fixed_rate_ninput_to_noutput(ninput_items - (ddc->history() - 1));
	gr_complex ddc_out[ddc_noutput_items];
	gr_vector_void_star ddc_out_vector(1);
	ddc_out_vector[0] = ddc_out;
	ddc_noutput_items = ddc->work(ddc_noutput_items, in, ddc_out_vector);

	
	int demod_noutput_items = ddc_noutput_items - 1;
	float demod_out[demod_noutput_items];
	demod(ddc_out, demod_out, demod_noutput_items);

	
	int cr_ninput_items = demod_noutput_items;
	int noutput_items = cr_ninput_items; // poor estimate but probably safe
	float cr_out[noutput_items];
	noutput_items = mm_cr(demod_out, cr_ninput_items, cr_out, noutput_items);

	
	slicer(cr_out, out, noutput_items);

	return noutput_items;
}


void bluetooth_multi_block::set_symbol_history(int num_symbols)
{
	set_history((int) (history() + (num_symbols * d_samples_per_symbol)));
}


void bluetooth_multi_block::set_channels()
{
	
	double center = (d_center_freq - BASE_FREQUENCY) / CHANNEL_WIDTH;
	
	double channel_bandwidth = d_sample_rate / CHANNEL_WIDTH;
	
	double low_edge = center - (channel_bandwidth / 2);
	
	double high_edge = center + (channel_bandwidth / 2);
	
	double min_channel_width = 0.9;

	d_low_channel = (int) (low_edge + (min_channel_width / 2) + 1);
	d_low_channel = (d_low_channel < 0) ? 0 : d_low_channel;

	d_high_channel = (int) (high_edge - (min_channel_width / 2));
	d_high_channel = (d_high_channel > 78) ? 78 : d_high_channel;
}


double bluetooth_multi_block::channel_freq(int channel)
{
	return BASE_FREQUENCY + (channel * CHANNEL_WIDTH) - d_center_freq;
}
