/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */



#ifndef BLUETOOTH_MULTI_BLOCK
#define BLUETOOTH_MULTI_BLOCK


class bluetooth_multi_block
{
protected:
	/* constructor */
	bluetooth_multi_block(double sample_rate, double center_freq, double squelch_threshold);

	
	static const int SYMBOL_RATE = 1000000;

	
	static const int SYMBOLS_PER_SLOT = 625;

	
	static const uint32_t BASE_FREQUENCY = 2402000000UL;

	
	static const int CHANNEL_WIDTH = 1000000;

	
	uint64_t d_cumulative_count;

	
	double d_sample_rate;

	
	double d_samples_per_symbol;

	
	double d_samples_per_slot;

	
	double d_center_freq;

	
	int d_low_channel;


	int d_high_channel;

	
	double d_squelch_threshold;

	
	int d_ddc_decimation_rate;

	
	float d_gain_mu;		
	float d_mu;				
	float d_omega_relative_limit;	
	float d_omega;			
	float d_gain_omega;		
	float d_omega_mid;		
	float d_last_sample;


	std::vector<float> d_channel_filter;

	
	float d_demod_gain;

	
	

	
	int mm_cr(const float *in, int ninput_items, float *out, int noutput_items);

	
	void demod(const gr_complex *in, float *out, int noutput_items);

	
	void slicer(const float *in, char *out, int noutput_items);

	
	int channel_symbols(int channel, gr_vector_const_void_star &in, char *out, int ninput_items);

	
	void set_symbol_history(int num_symbols);

	
	void set_channels();

	
	double channel_freq(int channel);
};

#endif 
