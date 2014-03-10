#ifndef WATER_MONITOR_H
#define WATER_MONITOR_H

#define OK 		1
#define FAIL 	0

#define COLLECT_PERIOD 	20 // In seconds
#define ACK_WAIT_TIME 	10 // In seconds

/**
 * Structure which contains sensors' read values.
 */
typedef struct data_s {
	uint16_t d1;
	uint16_t d2;
	uint16_t d3;
	uint16_t d4;
} data_t;

/**
 * Structure that is used to send all collected data together.
 */
typedef struct packet_s {
	uint16_t serial;
	struct data_s vet[8];
} packet_t;

/**
 * ACK structure.
 */
typedef struct ack_s {
	uint8_t   ack;   // always 1
	uint16_t  serial;
} ack_t;

#endif
