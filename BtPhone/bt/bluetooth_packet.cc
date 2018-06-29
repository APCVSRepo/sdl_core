/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */




#include "include/bluetooth_packet.h"
#include <stdlib.h>
#include <iostream>


const uint8_t bluetooth_packet::INDICES[] = {99, 85, 17, 50, 102, 58, 108, 45, 92, 62, 32, 118, 88, 11, 80, 2, 37, 69, 55, 8, 20, 40, 74, 114, 15, 106, 30, 78, 53, 72, 28, 26, 68, 7, 39, 113, 105, 77, 71, 25, 84, 49, 57, 44, 61, 117, 10, 1, 123, 124, 22, 125, 111, 23, 42, 126, 6, 112, 76, 24, 48, 43, 116, 0};

const uint8_t bluetooth_packet::WHITENING_DATA[] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1};

const uint8_t bluetooth_packet::PREAMBLE_DISTANCE[] = {2,2,1,2,2,1,2,2,1,2,0,1,2,2,1,2,2,1,2,2,1,0,2,1,2,2,1,2,2,1,2,2};

const uint8_t bluetooth_packet::TRAILER_DISTANCE[] = {3,3,3,2,3,2,2,1,2,3,3,3,3,3,3,2,2,3,3,3,3,3,3,2,1,2,2,3,2,3,3,3,3,2,2,1,2,1,1,0,3,3,3,2,3,2,2,1,3,3,3,2,3,2,2,1,2,3,3,3,3,3,3,2,2,3,3,3,3,3,3,2,1,2,2,3,2,3,3,3,1,2,2,3,2,3,3,3,0,1,1,2,1,2,2,3,3,3,3,2,3,2,2,1,2,3,3,3,3,3,3,2,2,3,3,3,3,3,3,2,1,2,2,3,2,3,3,3};

const string bluetooth_packet::TYPE_NAMES[] = {
	"NULL", "POLL", "FHS", "DM1", "DH1/2-DH1", "HV1", "HV2/2-EV3", "HV3/EV3/3-EV3",
	"DV/3-DH1", "AUX1", "DM3/2-DH3", "DH3/3-DH3", "EV4/2-EV5", "EV5/3-EV5", "DM5/2-DH5", "DH5/3-DH5"
};

/*
 * Create a new instance of bluetooth_packet
 */
bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length)
{
	return bluetooth_packet_sptr (new bluetooth_packet (stream, length));
}


bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length, uint32_t clkn, int channel)
{
	bluetooth_packet_sptr pkt = bluetooth_packet_sptr
			(new bluetooth_packet (stream, length));

	pkt->d_clkn = clkn;
	pkt->d_channel = channel;

	return pkt;
}

/* constructor */
bluetooth_packet::bluetooth_packet(char *stream, int length)
{
	int i;

	if(length > MAX_SYMBOLS)
		length = MAX_SYMBOLS;
	for(i = 0; i < length; i++)
		d_symbols[i] = stream[i];

	
	d_LAP = air_to_host32(&d_symbols[38], 24);
	d_length = length;
	d_whitened = true;
	d_have_UAP = false;
	d_have_NAP = false;
	d_have_clk6 = false;
	d_have_clk27 = false;
	d_have_payload = false;
	d_payload_length = 0;
}


int bluetooth_packet::sniff_ac(char *stream, int stream_length)
{
	// Looks for an AC in the stream 
	int count;
	uint8_t preamble; 
	uint16_t trailer; 
	int max_distance = 2; // maximum number of bit errors
	uint32_t LAP;
	char *symbols;

	for(count = 0; count < stream_length; count ++)
	{
		symbols = &stream[count];
		preamble = air_to_host8(&symbols[0], 5);
		trailer = air_to_host16(&symbols[61], 7);
		if((PREAMBLE_DISTANCE[preamble] + TRAILER_DISTANCE[trailer]) <= max_distance)
		{
			LAP = air_to_host32(&symbols[38], 24);
			if(check_ac(symbols, LAP))
			{
				return count;
			}
		}
	}
	return -1;
}

/* destructor */
bluetooth_packet::~bluetooth_packet()
{
}


uint8_t *bluetooth_packet::lfsr(uint8_t *data, int length, int k, uint8_t *g)
{
	int    i, j;
	uint8_t *cw, feedback;
	cw = (uint8_t *) calloc(length - k, 1);

	for (i = k - 1; i >= 0; i--) {
		feedback = data[i] ^ cw[length - k - 1];
		if (feedback != 0) {
			for (j = length - k - 1; j > 0; j--)
				if (g[j] != 0)
					cw[j] = cw[j - 1] ^ feedback;
				else
					cw[j] = cw[j - 1];
			cw[0] = g[0] && feedback;
		} else {
			for (j = length - k - 1; j > 0; j--)
				cw[j] = cw[j - 1];
			cw[0] = 0;
		}
	}
	return cw;
}


uint8_t bluetooth_packet::reverse(char byte)
{
	return (byte & 0x80) >> 7 | (byte & 0x40) >> 5 | (byte & 0x20) >> 3 | (byte & 0x10) >> 1 | (byte & 0x08) << 1 | (byte & 0x04) << 3 | (byte & 0x02) << 5 | (byte & 0x01) << 7;
}


uint8_t *bluetooth_packet::acgen(int LAP)
{
	
	uint8_t *retval, count, *cw, *data;
	retval = (uint8_t *) calloc(9,1);
	data = (uint8_t *) malloc(30);
	
	uint8_t pn[] = {0x03,0xF2,0xA3,0x3D,0xD6,0x9B,0x12,0x1C,0x10};
	
	uint8_t g[] = {1,0,0,1,0,1,0,1,1,0,1,1,1,1,0,0,1,0,0,0,1,1,1,0,1,0,1,0,0,0,0,1,1,0,1};

	LAP = reverse((LAP & 0xff0000)>>16) | (reverse((LAP & 0x00ff00)>>8)<<8) | (reverse(LAP & 0x0000ff)<<16);

	retval[4] = (LAP & 0xc00000)>>22;
	retval[5] = (LAP & 0x3fc000)>>14;
	retval[6] = (LAP & 0x003fc0)>>6;
	retval[7] = (LAP & 0x00003f)<<2;

	
	if(LAP & 0x1)
	{	retval[7] |= 0x03;
		retval[8] = 0x2a;
	} else
		retval[8] = 0xd5;

	for(count = 4; count < 9; count++)
		retval[count] ^= pn[count];

	data[0] = (retval[4] & 0x02) >> 1;
	data[1] = (retval[4] & 0x01);
	host_to_air(reverse(retval[5]), (char *) data+2, 8);
	host_to_air(reverse(retval[6]), (char *) data+10, 8);
	host_to_air(reverse(retval[7]), (char *) data+18, 8);
	host_to_air(reverse(retval[8]), (char *) data+26, 4);

	cw = lfsr(data, 64, 30, g);
	free(data);

	retval[0] = cw[0] << 3 | cw[1] << 2 | cw[2] << 1 | cw[3];
	retval[1] = cw[4] << 7 | cw[5] << 6 | cw[6] << 5 | cw[7] << 4 | cw[8] << 3 | cw[9] << 2 | cw[10] << 1 | cw[11];
	retval[2] = cw[12] << 7 | cw[13] << 6 | cw[14] << 5 | cw[15] << 4 | cw[16] << 3 | cw[17] << 2 | cw[18] << 1 | cw[19];
	retval[3] = cw[20] << 7 | cw[21] << 6 | cw[22] << 5 | cw[23] << 4 | cw[24] << 3 | cw[25] << 2 | cw[26] << 1 | cw[27];
	retval[4] = cw[28] << 7 | cw[29] << 6 | cw[30] << 5 | cw[31] << 4 | cw[32] << 3 | cw[33] << 2 | (retval[4] & 0x3);
	free(cw);

	for(count = 0; count < 9; count++)
		retval[count] ^= pn[count];

	
	if(retval[0] & 0x08)
		retval[0] |= 0xa0;
	else
		retval[0] |= 0x50;

	return retval;
}


void bluetooth_packet::convert_to_grformat(uint8_t input, uint8_t *output)
{
	int count;
	for(count = 0; count < 8; count++)
	{
		output[count] = (input & 0x80) >> 7;
		input <<= 1;
	}
}


bool bluetooth_packet::unfec13(char *input, char *output, int length)
{
	int a, b, c, i;
	int be = 0; // bit errors 

	for (i = 0; i < length; i++) {
		a = 3 * i;
		b = a + 1;
		c = a + 2;
		output[i] = ((input[a] & input[b]) | (input[b] & input[c]) |
				(input[c] & input[a]));
		be += ((input[a] ^ input[b]) | (input[b] ^ input[c]) |
				(input[c] ^ input[a]));
	}

	return (be < (length / 4));
}


char *bluetooth_packet::unfec23(char *input, int length)
{
	
	int iptr, optr, blocks;
	char* output;
	uint8_t difference, count, *codeword;
	uint8_t fecgen[] = {1,1,0,1,0,1};

	iptr = -15;
	optr = -10;
	difference = length % 10;
	// padding at end of data
	if(0!=difference)
		length += (10 - difference);

	blocks = length/10;
	output = (char *) malloc(length);

	while(blocks) {
		iptr += 15;
		optr += 10;
		blocks--;

		// copy data to output
		for(count=0;count<10;count++)
			output[optr+count] = input[iptr+count];

		
		codeword = lfsr((uint8_t *) input+iptr, 15, 10, fecgen);

		
		difference = 0;
		for(count=0;count<5;count++)
			if(codeword[count]!=input[iptr+10+count])
				difference++;

		
		if((0==difference) || (1==difference)) {
		    free(codeword);
		    continue;
		}

		
		for(count=0;count<5;count++) {
			difference |= codeword[count] ^ input[iptr+10+count];
			difference <<= 1;
		}
		free(codeword);

		switch (difference) {
			case 26: output[optr] ^= 1; break;
			
			case 13: output[optr+1] ^= 1; break;
			
			case 28: output[optr+2] ^= 1; break;
			
			case 14: output[optr+3] ^= 1; break;
			
			case 7: output[optr+4] ^= 1; break;
			
			case 25: output[optr+5] ^= 1; break;
			
			case 22: output[optr+6] ^= 1; break;
			
			case 11: output[optr+7] ^= 1; break;
			
			case 31: output[optr+8] ^= 1; break;
			
			case 21: output[optr+9] ^= 1; break;
			
			default: free(output); return false;
		}
	}
	return output;
}


bool bluetooth_packet::check_ac(char *stream, int LAP)
{
	int count, aclength, biterrors;
	uint8_t *ac, *grdata;
	aclength = 72;
	biterrors = 0;

	
	ac = acgen(LAP);

	
	grdata = (uint8_t *) malloc(aclength);

	for(count = 0; count < 9; count++)
		convert_to_grformat(ac[count], &grdata[count*8]);
	free(ac);

	for(count = 0; count < 68; count++)
	{
		if(grdata[count] != stream[count])
			biterrors++;
			
		if(biterrors>=7)
		{
			free(grdata);
			return false;
		}
	}
	if(biterrors)
	{
		
		free(grdata);
		
		return true;
	}

	free(grdata);
	return true;
}


uint8_t bluetooth_packet::air_to_host8(char *air_order, int bits)
{
	int i;
	uint8_t host_order = 0;
	for (i = 0; i < bits; i++)
		host_order |= (air_order[i] << i);
	return host_order;
}
uint16_t bluetooth_packet::air_to_host16(char *air_order, int bits)
{
	int i;
	uint16_t host_order = 0;
	for (i = 0; i < bits; i++)
		host_order |= (air_order[i] << i);
	return host_order;
}
uint32_t bluetooth_packet::air_to_host32(char *air_order, int bits)
{
	int i;
	uint32_t host_order = 0;
	for (i = 0; i < bits; i++)
		host_order |= (air_order[i] << i);
	return host_order;
}


void bluetooth_packet::host_to_air(uint8_t host_order, char *air_order, int bits)
{
    int i;
    for (i = 0; i < bits; i++)
        air_order[i] = (host_order >> i) & 0x01;
}


void bluetooth_packet::unwhiten(char* input, char* output, int clock, int length, int skip)
{
	int count, index;
	index = INDICES[clock & 0x3f];
	index += skip;
	index %= 127;

	for(count = 0; count < length; count++)
	{
		output[count] = (d_whitened) ? input[count] ^ WHITENING_DATA[index] : input[count];
		index += 1;
		index %= 127;
	}
}


uint16_t bluetooth_packet::crcgen(char *payload, int length, int UAP)
{
	char byte;
	uint16_t reg, count;

	reg = (reverse(UAP) << 8) & 0xff00;
	for(count = 0; count < length; count++)
	{
		byte = payload[count];

		reg = (reg >> 1) | (((reg & 0x0001) ^ (byte & 0x01))<<15);

		reg ^= ((reg & 0x8000)>>5);

		reg ^= ((reg & 0x8000)>>12);
	}
	return reg;
}


uint32_t bluetooth_packet::get_LAP()
{
	return d_LAP;
}


uint8_t bluetooth_packet::get_UAP()
{
	return d_UAP;
}


void bluetooth_packet::set_UAP(uint8_t UAP)
{
	d_UAP = UAP;
	d_have_UAP = true;
}

void bluetooth_packet::set_NAP(uint16_t NAP)
{
	d_NAP = NAP;
	d_have_NAP = true;
}


uint32_t bluetooth_packet::get_clock()
{
	return d_clock;
}


void bluetooth_packet::set_clock(uint32_t clock, bool have27)
{
	if (have27)
		d_clock = clock & 0x7ffffff;
	else
		d_clock = clock & 0x3f;

	d_have_clk6 = true;
	d_have_clk27 = have27;
}


bool bluetooth_packet::get_whitened()
{
	return d_whitened;
}


void bluetooth_packet::set_whitened(bool whitened)
{
	d_whitened = whitened;
}

int bluetooth_packet::UAP_from_hec(uint16_t data, uint8_t hec)
{
        int i;

        for (i = 9; i >= 0; i--) {
                
                if (hec & 0x80)
                        hec ^= 0x65;

                hec = (hec << 1) | (((hec >> 7) ^ (data >> i)) & 0x01);
        }
        return reverse(hec);
}


int bluetooth_packet::crc_check(int clock)
{
	
	int retval = 1;

	switch(d_packet_type)
	{
		case 2:
			retval = fhs(clock);
			break;

		case 8:
		case 3:
		case 10:
		case 14:
			retval = DM(clock);
			break;

		case 4:
		case 11:
		case 15:
			retval = DH(clock);
			break;

		case 7:
			retval = EV3(clock);
			break;
		case 12:
			retval = EV4(clock);
			break;
		case 13:
			retval = EV5(clock);
			break;
		
		case 5:
			retval = HV(clock);
			break;

		
		default:
			break;
	}
	
	if (retval == 0 && (d_packet_type != 2 && d_packet_type != 3 &&
			d_packet_type != 5))
		return 1;

	
	if (retval > 1 && (d_packet_type == 7 || d_packet_type == 13))
		return 1;

	return retval;
}


bool bluetooth_packet::payload_crc()
{
	uint16_t crc;   
	uint16_t check; 

	crc = crcgen(d_payload, (d_payload_length - 2) * 8, d_UAP);
	check = air_to_host16(&d_payload[(d_payload_length - 2) * 8], 16);

	return (crc == check);
}

int bluetooth_packet::fhs(int clock)
{
	
	char *stream = d_symbols + 126;
	
	int size = d_length - 126;

	d_payload_length = 20;

	if (size < d_payload_length * 12)
		return 1; 

	char *corrected = unfec23(stream, d_payload_length * 8);
	if (!corrected)
		return 0;

	
	unwhiten(corrected, d_payload, clock, d_payload_length * 8, 18);
	if (payload_crc()) {
		free(corrected);
		return 1000;
	}

	
	for (clock = 32; clock < 64; clock++) {
		unwhiten(corrected, d_payload, clock, d_payload_length * 8, 18);
		if (payload_crc()) {
			free(corrected);
			return 1000;
		}
	}

	
	free(corrected);
	return 0;
}


bool bluetooth_packet::decode_payload_header(char *stream, int clock, int header_bytes, int size, bool fec)
{
	if(header_bytes == 2)
	{
		if(size < 16)
			return false; 
		if(fec) {
			if(size < 30)
				return false; 
			char *corrected = unfec23(stream, 16);
			if (!corrected)
				return false;
			unwhiten(corrected, d_payload_header, clock, 16, 18);
			free(corrected);
		} else {
			unwhiten(stream, d_payload_header, clock, 16, 18);
		}
		
		d_payload_length = air_to_host16(&d_payload_header[3], 10) + 4;
	} else {
		if(size < 8)
			return false; 
		if(fec) {
			if(size < 15)
				return false; 
			char *corrected = unfec23(stream, 8);
			if (!corrected)
				return false;
			unwhiten(corrected, d_payload_header, clock, 8, 18);
			free(corrected);
		} else {
			unwhiten(stream, d_payload_header, clock, 8, 18);
		}
		
		d_payload_length = air_to_host8(&d_payload_header[3], 5) + 3;
	}
	d_payload_llid = air_to_host8(&d_payload_header[0], 2);
	d_payload_flow = air_to_host8(&d_payload_header[2], 1);
	d_payload_header_length = header_bytes;
	return true;
}


int bluetooth_packet::DM(int clock)
{
	int bitlength;
	
	int header_bytes = 2;
	
	int max_length;
	
	char *stream = d_symbols + 126;
	
	int size = d_length - 126;

	switch(d_packet_type)
	{
		case(8): 
			
			stream += 80;
			size -= 80;
			header_bytes = 1;
			
			max_length = 12;
			break;
		case(3): 
			header_bytes = 1;
			max_length = 20;
			break;
		case(10): 
			max_length = 125;
			break;
		case(14): 
			max_length = 228;
			break;
		default: 
			return 0;
	}
	if(!decode_payload_header(stream, clock, header_bytes, size, true))
		return 0;
	
	if(d_payload_length > max_length)
		
		return 1;
	bitlength = d_payload_length*8;
	if(bitlength > size)
		return 1; 

	char *corrected = unfec23(stream, bitlength);
	if (!corrected)
		return 0;
	unwhiten(corrected, d_payload, clock, bitlength, 18);
	free(corrected);

	if (payload_crc())
		return 10;

	
	return 1;
}


int bluetooth_packet::DH(int clock)
{
	int bitlength;
	
	int header_bytes = 2;
	
	int max_length;
	
	char *stream = d_symbols + 126;
	
	int size = d_length - 126;
	
	switch(d_packet_type)
	{
		case(9): 
		case(4): 
			header_bytes = 1;
			max_length = 30;
			break;
		case(11): 
			max_length = 187;
			break;
		case(15): 
			max_length = 343;
			break;
		default: 
			return 0;
	}
	if(!decode_payload_header(stream, clock, header_bytes, size, false))
		return 0;
	
	if(d_payload_length > max_length)
		
		return 1;
	bitlength = d_payload_length*8;
	if(bitlength > size)
		return 1; 

	unwhiten(stream, d_payload, clock, bitlength, 18);
	
	
	if (d_packet_type == 9)
		return 1;

	if (payload_crc())
		return 10;

	
	return 1;
}

int bluetooth_packet::EV3(int clock)
{
	
	char *stream = d_symbols + 126;

	
	int size = d_length - 126;

	
	int maxlength = 32;

	
	int bits;

	
	for (d_payload_length = 0;
			d_payload_length < maxlength; d_payload_length++) {

		bits = d_payload_length * 8;

		
		if ((bits + 8) > size)
			return 1; 
		unwhiten(stream, d_payload + bits, clock, 8, 18 + bits);

		if ((d_payload_length > 2) && (payload_crc()))
				return 10;
	}
	return 1;
}

int bluetooth_packet::EV4(int clock)
{
	char *corrected;

	
	char *stream = d_symbols + 126;

	
	int size = d_length - 126;

	
	int maxlength = 1470;

	
	int minlength = 45;

	int syms = 0; 
	int bits = 0; 

	d_payload_length = 1;

	while (syms < maxlength) {

	
		if (syms + 15 > size)
			return 1; 
		corrected = unfec23(stream + syms, 10);
		if (!corrected) {
			free(corrected);
			if (syms < minlength)
				return 0;
			else
				return 1;
		}
		unwhiten(corrected, d_payload + bits, clock, 10, 18 + bits);
		free(corrected);

		
		while (d_payload_length * 8 <= bits) {
			if (payload_crc())
				return 10;
			d_payload_length++;
		}
		syms += 15;
		bits += 10;
	}
	return 1;
}

int bluetooth_packet::EV5(int clock)
{
	
	char *stream = d_symbols + 126;

	
	int size = d_length - 126;

	
	int maxlength = 182;

	
	int bits;

	
	for (d_payload_length = 0;
			d_payload_length < maxlength; d_payload_length++) {

		bits = d_payload_length * 8;

		
		if ((bits + 8) > size)
			return 1; 
		unwhiten(stream, d_payload + bits, clock, 8, 18 + bits);

		if ((d_payload_length > 2) && (payload_crc()))
				return 10;
	}
	return 1;
}


int bluetooth_packet::HV(int clock)
{
	
	char *stream = d_symbols + 126;
	
	int size = d_length - 126;

	if(size < 240) {
		d_payload_length = 0;
		return 1; 
	}

	switch (d_packet_type) {
	case 5:
		{
		char corrected[80];
		if (!unfec13(stream, corrected, 80))
			return 0;
		d_payload_length = 10;
		unwhiten(corrected, d_payload, clock, d_payload_length*8, 18);
		}
		break;
	case 6:
		{
		char *corrected = unfec23(stream, 160);
		if (!corrected)
			return 0;
		d_payload_length = 20;
		unwhiten(corrected, d_payload, clock, d_payload_length*8, 18);
		free(corrected);
		}
		break;
	case 7:
		d_payload_length = 30;
		unwhiten(stream, d_payload, clock, d_payload_length*8, 18);
		break;
	}

	return 1;
}

uint8_t bluetooth_packet::try_clock(int clock)
{
	
	char *stream = d_symbols + 72;
	
	char header[18];
	char unwhitened[18];

	if (!unfec13(stream, header, 18))
		return 0;
	unwhiten(header, unwhitened, clock, 18, 0);
	uint16_t hdr_data = air_to_host16(unwhitened, 10);
	uint8_t hec = air_to_host8(&unwhitened[10], 8);
	d_UAP = bluetooth_packet::UAP_from_hec(hdr_data, hec);
	d_packet_type = air_to_host8(&unwhitened[3], 4);

	return d_UAP;
}

// decode the packet header 
bool bluetooth_packet::decode_header()
{
	
	char *stream = d_symbols + 72;
	
	char header[18];
	uint8_t UAP;

	if (d_have_clk6 && unfec13(stream, header, 18)) {
		unwhiten(header, d_packet_header, d_clock, 18, 0);
		uint16_t hdr_data = air_to_host16(d_packet_header, 10);
		uint8_t hec = air_to_host8(&d_packet_header[10], 8);
		UAP = bluetooth_packet::UAP_from_hec(hdr_data, hec);
		if (UAP == d_UAP) {
			d_packet_type = air_to_host8(&d_packet_header[3], 4);
			return true;
		} else {
			printf("bad HEC! ");
		}
	}
	
	printf("failed to decode header\n");
	return false;
}

void bluetooth_packet::decode_payload()
{
	d_payload_header_length = 0;

	switch(d_packet_type)
	{
		case 0: 
			
			d_payload_length = 0;
			break;
		case 1: 
			
			d_payload_length = 0;
			break;
		case 2: 
			fhs(d_clock);
			break;
		case 3: 
			DM(d_clock);
			break;
		case 4: 
			
			DH(d_clock);
			break;
		case 5: 
			HV(d_clock);
			break;
		case 6: 
			HV(d_clock);
			break;
		case 7: 
			
			if (EV3(d_clock) <= 1)
				
				HV(d_clock);
			
			break;
		case 8: 
			
			DM(d_clock);
			break;
		case 9: 
			DH(d_clock);
			break;
		case 10: 
			
			DM(d_clock);
			break;
		case 11: 
			
			DH(d_clock);
			break;
		case 12: 
			
			EV4(d_clock);
			break;
		case 13: 
			
			EV5(d_clock);
		case 14: 
			
			DM(d_clock);
			break;
		case 15: 
			
			DH(d_clock);
			break;
	}
	d_have_payload = true;
}


void bluetooth_packet::decode()
{
	d_have_payload = false;
	if (decode_header())
		decode_payload();
}


void bluetooth_packet::print()
{
	if (d_have_payload) {
		cout << TYPE_NAMES[d_packet_type] << endl;
		if (d_payload_header_length > 0) {
			printf("  LLID: %d\n", d_payload_llid);
			printf("  flow: %d\n", d_payload_flow);
			printf("  payload length: %d\n", d_payload_length);
		}
	}
}

char *bluetooth_packet::tun_format()
{
	
	int length = 9 + d_payload_length;
	char *tun_format = (char *) malloc(length);
	int i;

	
	tun_format[0] = d_clock & 0xff;
	tun_format[1] = (d_clock >> 8) & 0xff;
	tun_format[2] = (d_clock >> 16) & 0xff;
	tun_format[3] = (d_clock >> 24) & 0xff;
	tun_format[4] = d_channel;
	tun_format[5] = d_have_clk27 | (d_have_NAP << 1);

	
	tun_format[6] = (char) air_to_host8(&d_packet_header[0], 7);
	
	tun_format[7] = (char) air_to_host8(&d_packet_header[7], 3);
	
	tun_format[8] = (char) air_to_host8(&d_packet_header[10], 8);

	for(i=0;i<d_payload_length;i++)
		tun_format[i+9] = (char) air_to_host8(&d_payload[i*8], 8);

	return tun_format;
}

bool bluetooth_packet::got_payload()
{
	return d_have_payload;
}

int bluetooth_packet::get_payload_length()
{
	return d_payload_length;
}

int bluetooth_packet::get_type()
{
	return d_packet_type;
}

// check to see if the packet has a header 
bool bluetooth_packet::header_present()
{
	
	char *stream = d_symbols + 67;
	int be = 0; 
	char msb;   
	int a, b, c;

	
	if (d_length < 126)
		return false;

	
	msb = stream[0];
	be += stream[1] ^ !msb;
	be += stream[2] ^ msb;
	be += stream[3] ^ !msb;
	be += stream[4] ^ msb;

	
	stream += 5;
	for (a = 0; a < 54; a += 3) {
		b = a + 1;
		c = a + 2;
		be += ((stream[a] ^ stream[b]) |
			(stream[b] ^ stream[c]) | (stream[c] ^ stream[a]));
	}

	
	return (be < ID_THRESHOLD);
}


uint32_t bluetooth_packet::lap_from_fhs()
{
	
	return air_to_host32(&d_payload[34], 24);
}


uint8_t bluetooth_packet::uap_from_fhs()
{
	
	return air_to_host8(&d_payload[64], 8);
}


uint16_t bluetooth_packet::nap_from_fhs()
{
	
	return air_to_host8(&d_payload[72], 16);
}


uint32_t bluetooth_packet::clock_from_fhs()
{
	
	return air_to_host32(&d_payload[115], 26);
}
