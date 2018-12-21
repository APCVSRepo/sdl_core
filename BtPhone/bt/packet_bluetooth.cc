/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */



#include "include/packet_bluetooth.h"


extern int pack_comp_bluetooth;

static int debugno = 0;

int kis_bluetooth_dissector(CHAINCALL_PARMS) {
	int offset = 0;

	bluetooth_packinfo *pi = NULL;

	if (in_pack->error)
		return 0;

	kis_datachunk *chunk =
		(kis_datachunk *) in_pack->fetch(_PCM(PACK_COMP_LINKFRAME));

	if (chunk == NULL)
		return 0;

	if (chunk->dlt != KDLT_BLUETOOTH)
		return 0;

	debugno++;

	if (chunk->length < 14) {
		_MSG("Short Bluetooth frame!", MSGFLAG_ERROR);
		in_pack->error = 1;
		return 0;
	}

	pi = new bluetooth_packinfo();

	pi->type = btbb_type_id;
	
	pi->lap = chunk->data[9] << 16;
	pi->lap |= chunk->data[10] << 8;
	pi->lap |= chunk->data[11];

	

	in_pack->insert(pack_comp_bluetooth, pi);

	return 1;
}
