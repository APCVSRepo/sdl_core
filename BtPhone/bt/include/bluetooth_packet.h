/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 * 
 */
 
#ifndef BLUETOOTH_PACKET_H
#define BLUETOOTH_PACKET_H
 

#include <stdint.h>
#include <string>
#include <cstdio>


using namespace std;

class bluetooth_packet;
typedef boost::shared_ptr<bluetooth_packet> bluetooth_packet_sptr;


bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length);


bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length,
		uint32_t clkn, int channel);


class bluetooth_packet
{
private:
	
	friend bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length);
	friend bluetooth_packet_sptr bluetooth_make_packet(char *stream, int length,
			uint32_t clkn, int channel);
	
	bluetooth_packet(char *stream, int length);
	
	static const int MAX_SYMBOLS = 3125;
	
	static const int ID_THRESHOLD = 5;

	static const uint8_t INDICES[64];

	static const uint8_t WHITENING_DATA[127];

	static const uint8_t PREAMBLE_DISTANCE[32];

	static const uint8_t TRAILER_DISTANCE[2048];

	static const string TYPE_NAMES[16];

	char d_symbols[MAX_SYMBOLS];

	uint32_t d_LAP;

	uint8_t d_UAP;

	uint16_t d_NAP;

	int d_length;

	int d_packet_type;

	char d_packet_header[18];

	char d_payload_header[16];

	int d_payload_header_length;

	uint8_t d_payload_llid;

	uint8_t d_payload_flow;

	int d_payload_length;

	char d_payload[2744];

	bool d_whitened;

	bool d_have_UAP;
	bool d_have_NAP;

	bool d_have_clk6;
	bool d_have_clk27;

	bool d_have_payload;

	uint32_t d_clock;

	int fhs(int clock);
	int DM(int clock);
	int DH(int clock);
	int EV3(int clock);
	int EV4(int clock);
	int EV5(int clock);
	int HV(int clock);

	bool decode_payload_header(char *stream, int clock, int header_bytes, int size, bool fec);

	void unwhiten(char* input, char* output, int clock, int length, int skip);

	bool payload_crc();

public:
	int d_channel;

	uint32_t d_clkn;

	static int sniff_ac(char *stream, int stream_length);

	static uint8_t *lfsr(uint8_t *data, int length, int k, uint8_t *g);

	static uint8_t reverse(char byte);

	static uint8_t *acgen(int LAP);

	static void convert_to_grformat(uint8_t input, uint8_t *output);

	static bool unfec13(char *input, char *output, int length);

	static char *unfec23(char *input, int length);

	static char *fec23gen(char *data);

	static bool check_ac(char *stream, int LAP);

	static uint8_t air_to_host8(char *air_order, int bits);
	static uint16_t air_to_host16(char *air_order, int bits);
	static uint32_t air_to_host32(char *air_order, int bits);
	static void host_to_air(uint8_t host_order, char *air_order, int bits);

	static uint16_t crcgen(char *payload, int length, int UAP);

	static int UAP_from_hec(uint16_t data, uint8_t hec);

	int crc_check(int clock);

	bool decode_header();

	void decode_payload();

	void decode();

	void print();

	char *tun_format();

	uint32_t get_LAP();

	uint8_t get_UAP();

	void set_UAP(uint8_t UAP);

	void set_NAP(uint16_t NAP);

	bool get_whitened();

	void set_whitened(bool whitened);

	bool got_payload();

	uint32_t get_clock();

	void set_clock(uint32_t clk6, bool have27);

	int get_payload_length();

	int get_type();

	uint8_t try_clock(int clock);

	bool header_present();

	uint32_t lap_from_fhs();

	uint8_t uap_from_fhs();

	uint16_t nap_from_fhs();

	uint32_t clock_from_fhs();


	~bluetooth_packet();
};

#endif
