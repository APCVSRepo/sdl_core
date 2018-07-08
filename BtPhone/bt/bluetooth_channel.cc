/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */


#include "include/bluetooth_channel.h"


//private constructor
bluetooth_channel::bluetooth_channel(double sample_rate, double center_freq, double squelch_threshold)
{
	set_symbol_history(68);
	pending_packet = 0;
	fake_fd[0] = -1;
	fake_fd[1] = -1;

	printf("lowest channel: %d, highest channel %d\n", d_low_channel, d_high_channel);
}

//virtual destructor.
bluetooth_channel::~bluetooth_channel ()
{
}

int bluetooth_channel::work(int noutput_items,
			       gr_vector_const_void_star &input_items,
			       gr_vector_void_star &output_items)
{
	int retval, channel;
	char symbols[history()];

	for (channel = d_low_channel; channel <= d_high_channel; channel++) {
		int num_symbols = channel_symbols(channel, input_items,
				symbols, history() /*+ noutput_items*/);

		
		if (num_symbols == 0)
			break;

		if (num_symbols >= 68 ) {
			
			int latest_ac = (num_symbols - 68) < 625 ? (num_symbols - 68) : 625;
			retval = bluetooth_packet::sniff_ac(symbols, latest_ac);
			if (retval > -1) {
				bluetooth_packet_sptr packet = bluetooth_make_packet(&symbols[retval], num_symbols - retval);
				enqueue(packet, channel);
				
			}
		}
	}
	d_cumulative_count += (int) d_samples_per_slot;

	
	return (int) d_samples_per_slot;
}

void bluetooth_channel::enqueue(bluetooth_packet_sptr pkt, int channel)
{
	
	char *data = new char[14];
	int len = 14;
	uint32_t lap = pkt->get_LAP();
	
	data[0] = data[1] = data[2] = data[3] = data[4] = data[5] = 0x00;
	data[6] = data[7] = data[8] = 0x00;
	data[9] = (lap >> 16) & 0xff;
	data[10] = (lap >> 8) & 0xff;
	data[11] = lap & 0xff;
	data[12] = 0xff;
	data[13] = 0xf0;

	
	pthread_mutex_lock(&packet_lock);

	if (packet_queue.size() > 20) {
		
	} else {
		
		struct usrp_bt_pkt *rpkt = new usrp_bt_pkt;
		rpkt->data = data;
		rpkt->len = len;
		rpkt->channel = channel;

		packet_queue.push_back(rpkt);
		if (pending_packet == 0) {
			// printf("debug - writing\n");
			pending_packet = 1;
			write(fake_fd[1], data, 1);
		}

	}
	pthread_mutex_unlock(&packet_lock);
}
