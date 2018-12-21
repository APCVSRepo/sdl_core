/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */

#ifndef BLUETOOTH_CHANNEL
#define BLUETOOTH_CHANNEL

#include <bluetooth_channel.h>
#include <bluetooth_packet.h>
#include <pthread.h>
#include <vector>




class bluetooth_channel
{
private:
	

	/* constructor */
	bluetooth_channel(double sample_rate, double center_freq, double squelch_threshold);

	void enqueue(bluetooth_packet_sptr packet, int channel);

public:
	/* destructor */
	~bluetooth_channel();

	struct usrp_bt_pkt {
		char *data;
		int len;
		int channel;
	};

	pthread_mutex_t packet_lock;

	
	vector<struct usrp_bt_pkt *> packet_queue;

	
	int pending_packet;

	
	int fake_fd[2];

	
	int work(int noutput_items,
		    gr_vector_const_void_star &input_items,
		    gr_vector_void_star &output_items);
};

#endif
