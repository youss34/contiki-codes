#ifndef SMARTHOME_H
#define SMARTHOME_H

/* --- Estruturas genericas --- */

typedef struct cmd_s {
	uint8_t id;
	uint8_t info;
} cmd_t;

struct cmdf_s {
	uint8_t id;
	float info;
} __attribute__((packed));

typedef struct cmdf_s cmdf_t;

/* --- Estruturas com atributos especificos --- 
---	 de cada aplicacao para retorno de status ---
*/

typedef struct tv_status_s {
	uint8_t id;
	uint8_t on_off;
	uint8_t channel;
	uint8_t volume;
} tv_status_t;

struct radio_status_s {
	uint8_t id;
	uint8_t on_off;
	float station;
	uint8_t volume;
} __attribute__((packed));

typedef struct radio_status_s radio_status_t;

struct termostato_status_s {
	uint8_t id;
	uint8_t on_off;
	float temperature;
} __attribute__((packed));

typedef struct termostato_status_s termostato_status_t;

struct fogao_status_s {
	uint8_t id;
	uint8_t status_boca1;
	uint8_t status_boca2;
	uint8_t status_boca3;
	uint8_t status_boca4;
	uint8_t status_forno;
	float temperature;
} __attribute__((packed));

typedef struct fogao_status_s fogao_status_t;

enum {
	CMD_TURN    		= 1,

	GET_STATUS 			= 2,	
	GET_CHANNEL			= 3,
	GET_VOLUME			= 4,
	GET_TEMPERATURE 	= 5,
	GET_STATION			= 6,

	SET_CHANNEL 		= 7,
	SET_VOLUME  		= 8,	
	SET_TEMPERATURE		= 9,	
	SET_STATION 		= 10,

	RESP_GET_STATUS 	= 11,
	RESP_GET_CHANNEL	= 12,
	RESP_GET_VOLUME		= 13,
	RESP_GET_TEMPERATURE= 14,
	RESP_GET_STATION	= 15,
};

enum {
	TURN_ON    = 1,
	TURN_OFF   = 2,
};

#endif
