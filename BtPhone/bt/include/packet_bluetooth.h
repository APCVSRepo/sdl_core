/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */
#ifndef __PACKET_BLUETOOTH_H__
#define __PACKET_BLUETOOTH_H__

#define KDLT_BLUETOOTH 1

int kis_bluetooth_dissector(CHAINCALL_PARMS);

enum bluetooth_type {
	btbb_type_null = 0x0,
	btbb_type_poll = 0x1,
	btbb_type_fhs = 0x2,
	btbb_type_dm1 = 0x3,
	btbb_type_dh1 = 0x4,
	btbb_type_hv1 = 0x5,
	btbb_type_hv2 = 0x6,
	btbb_type_hv3 = 0x7,
	btbb_type_dv = 0x8,
	btbb_type_aux1 = 0x9,
	btbb_type_dm3 = 0xa,
	btbb_type_dh3 = 0xb,
	btbb_type_ev4 = 0xc,
	btbb_type_ev5 = 0xd,
	btbb_type_dm5 = 0xe,
	btbb_type_dh5 = 0xf,
	btbb_type_id,
	btbb_type_max
};
extern const char *bluetooth_type_str[];

class bluetooth_packinfo : public packet_component {
public:
	bluetooth_packinfo() {
		self_destruct = 1;

		lap = 0;
		uap = 0;
		nap = 0;
		have_uap = false;
		have_nap = false;
		type = btbb_type_id;

		channel = 0;
	};

	uint32_t lap;
	uint8_t uap;
	uint16_t nap;

	bool have_uap;
	bool have_nap;

	bluetooth_type type;

	int channel;

};

#endif