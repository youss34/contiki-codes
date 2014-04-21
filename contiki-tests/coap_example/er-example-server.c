#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"


/* Define which resources to include to meet memory constraints. */
#define REST_RES_CHUNKS 1
#define REST_RES_LEDS 1
#define REST_RES_TOGGLE 1
#define REST_RES_SUM 1
#define REST_RES_TIMES 1

#include "erbium.h"

#if defined (PLATFORM_HAS_BUTTON)
#include "dev/button-sensor.h"
#endif
#if defined (PLATFORM_HAS_LEDS)
#include "dev/leds.h"
#endif
#if defined (PLATFORM_HAS_LIGHT)
#include "dev/light-sensor.h"
#endif

/* For CoAP-specific example: not required for normal RESTful Web service. */
#if WITH_COAP == 3
#include "er-coap-03.h"
#elif WITH_COAP == 7
#include "er-coap-07.h"
#elif WITH_COAP == 12
#include "er-coap-12.h"
#elif WITH_COAP == 13
#include "er-coap-13.h"
#else
#warning "Erbium example without CoAP-specifc functionality"
#endif /* CoAP-specific example */

#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF("[%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x]", ((uint8_t *)addr)[0], ((uint8_t *)addr)[1], ((uint8_t *)addr)[2], ((uint8_t *)addr)[3], ((uint8_t *)addr)[4], ((uint8_t *)addr)[5], ((uint8_t *)addr)[6], ((uint8_t *)addr)[7], ((uint8_t *)addr)[8], ((uint8_t *)addr)[9], ((uint8_t *)addr)[10], ((uint8_t *)addr)[11], ((uint8_t *)addr)[12], ((uint8_t *)addr)[13], ((uint8_t *)addr)[14], ((uint8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF("[%02x:%02x:%02x:%02x:%02x:%02x]",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

/******************************************************************************/
#if REST_RES_CHUNKS

RESOURCE(chunks, METHOD_GET, "test/chunks", "title=\"Blockwise demo\";rt=\"Data\"");

#define CHUNKS_TOTAL	2050

void chunks_handler(void* request, void* response, uint8_t *buffer,
							uint16_t preferred_size, int32_t *offset)
{
	int32_t strpos = 0;

	/* Check the offset for boundaries of the resource data. */
	if (*offset>=CHUNKS_TOTAL){
		REST.set_response_status(response, REST.status.BAD_OPTION);
		//A block error message should not exceed the minimum block size (16).

		const char *error_msg = "BlockOutOfScope";
		REST.set_response_payload(response, error_msg, strlen(error_msg));
		return;
	}

	/* Generate data until reaching CHUNKS_TOTAL. */
	while (strpos<preferred_size){
		strpos += snprintf((char *)buffer+strpos, preferred_size-strpos+1,
								"|%ld|", *offset);
	}

	/* snprintf() does not adjust return value if truncated by size. */
	if (strpos > preferred_size){
		strpos = preferred_size;
	}

	/* Truncate if above CHUNKS_TOTAL bytes. */
	if (*offset+(int32_t)strpos > CHUNKS_TOTAL){
		strpos = CHUNKS_TOTAL - *offset;
	}

	REST.set_response_payload(response, buffer, strpos);

	//IMPORTANT for chunk-wise resources: Signal chunk awareness to REST engine.
	*offset += strpos;

	/* Signal end of resource representation. */
	if (*offset>=CHUNKS_TOTAL){
		*offset = -1;
	}
}
#endif

/******************************************************************************/
#if defined (PLATFORM_HAS_LEDS)
/******************************************************************************/
#if REST_RES_LEDS
RESOURCE(leds, METHOD_POST | METHOD_PUT , "actuators/leds",
		"title=\"LEDs: ?color=r|g|b, POST/PUT mode=on|off\";rt=\"Control\"");

void leds_handler(void* request, void* response, uint8_t *buffer,
						uint16_t preferred_size, int32_t *offset)
{
	coap_packet_t *in = (coap_packet_t *)request;
	char q[in->uri_query_len + 1];
	strncpy(q, in->uri_query, in->uri_query_len);
	q[in->uri_query_len] = '\0';
	//printf("Query = %s\n", q);

	const char s[4] = "=;&";
	char *token = NULL;
	int success = 1;
	size_t led = 0;

	token = strtok(q, s);
	while(token != NULL){
		if(strcmp(token, "color") == 0){
			token = strtok(NULL, s);
			if(token == NULL){
				success = 0;
				break;
			}
			if(strcmp(token, "g") == 0){
				led = LEDS_GREEN;
			}
			else if(strcmp(token, "r") == 0){
				led = LEDS_RED;
			}
			else if(strcmp(token, "b") == 0){
				led = LEDS_BLUE;
			}
			else{
				success = 0;
				break;
			}
		}
		else if(strcmp(token, "mode") == 0){
			token = strtok(NULL, s);
			if(token == NULL){
				success = 0;
				break;
			}
			if(strcmp(token, "on") == 0){
				leds_on(led);
			}
			else if(strcmp(token, "off") == 0){
				leds_off(led);
			}
			else{
				success = 0;
				break;
			}
		}
		else{
			success = 0;
			break;
		}
		// Next variable
		token = strtok(NULL, s);
	}
	
	if (!success) {
		REST.set_response_status(response, REST.status.BAD_REQUEST);
	}
}
#endif

/******************************************************************************/
#if REST_RES_TOGGLE
/* A simple actuator example. Toggles the red led */
RESOURCE(toggle, METHOD_POST, "actuators/toggle",
				"title=\"Red LED\";rt=\"Control\"");
void toggle_handler(void* request, void* response, uint8_t *buffer,
						uint16_t preferred_size, int32_t *offset)
{
	leds_toggle(LEDS_RED);
}
#endif
#endif /* PLATFORM_HAS_LEDS */

#if REST_RES_SUM

static int amount = 0;

RESOURCE(sum, METHOD_GET | METHOD_POST, "services/sum",
				"title=\"SUM: ?num1=integer, num2=integer\";rt=\"Service\"");

void sum_handler(void* request, void* response, uint8_t *buffer,
						uint16_t preferred_size, int32_t *offset)
{
	coap_packet_t *in = (coap_packet_t *)request;

	if(in->code == COAP_GET){
		char res[REST_MAX_CHUNK_SIZE];
		coap_set_header_content_type(response, TEXT_PLAIN);
		snprintf(res, REST_MAX_CHUNK_SIZE, "%d", amount);
		coap_set_payload(response, res, strlen(res));
		return;
	}
	
	char q[in->uri_query_len + 1];
	strncpy(q, in->uri_query, in->uri_query_len);
	q[in->uri_query_len] = '\0';
	//printf("Query = %s\n", q);

	const char s[4] = "=;&";
	char *token = NULL;
	int num1 = 0, num2 = 0;
	short success = 1, count = 0;
	
	token = strtok(q, s);
	while(token != NULL && count < 2){
		if(strcmp(token, "num1") == 0){
			token = strtok(NULL, s);
			if(token == NULL){
				success = 0;
				break;
			}
			num1 = atoi(token);
			count++;
		}
		else if(strcmp(token, "num2") == 0){
			token = strtok(NULL, s);
			if(token == NULL){
				success = 0;
				break;
			}
			num2 = atoi(token);
			count++;
		}
		else{
			success = 0;
			break;
		}
		// Next variable
		token = strtok(NULL, s);
	}

	if(success){
		amount = num1 + num2;
		//printf("amount = %d\n", amount);
	}
	else{
		REST.set_response_status(response, REST.status.BAD_REQUEST);
	}
}
#endif

#if REST_RES_TIMES

RESOURCE(times, METHOD_POST, "services/times","title=\"TIMES\";rt=\"Service\"");

void times_handler(void* request, void* response, uint8_t *buffer,
						uint16_t preferred_size, int32_t *offset)
{
	coap_packet_t *in = (coap_packet_t *)request;
	uint16_t len = in->payload_len;
	char p[len + 1];
	memcpy(p, in->payload, len * sizeof(char));
	p[len] = '\0';
	//printf("Payload = %s\n", p);

	const char s[4] = "=;&";
	char *token = NULL;
	int num1 = 0, num2 = 0;
	short success = 1, count = 0;
	
	token = strtok(p, s);
	while(token != NULL && count < 2){
		if(strcmp(token, "num1") == 0){
			token = strtok(NULL, s);
			if(token == NULL){
				success = 0;
				break;
			}
			num1 = atoi(token);
			count++;
		}
		else if(strcmp(token, "num2") == 0){
			token = strtok(NULL, s);
			if(token == NULL){
				success = 0;
				break;
			}
			num2 = atoi(token);
			count++;
		}
		else{
			success = 0;
			break;
		}
		// Next variable
		token = strtok(NULL, s);
	}

	if(success){
		num1 *= num2;
		//printf("result = %d\n", num1);
		char res[REST_MAX_CHUNK_SIZE];
		coap_set_header_content_type(response, TEXT_PLAIN);
		snprintf(res, REST_MAX_CHUNK_SIZE, "%d", num1);
		coap_set_payload(response, res, strlen(res));
	}
	else{
		REST.set_response_status(response, REST.status.BAD_REQUEST);
	}
}
#endif

PROCESS(rest_server_example, "Erbium Example Server");
AUTOSTART_PROCESSES(&rest_server_example);

PROCESS_THREAD(rest_server_example, ev, data)
{
	PROCESS_BEGIN();

	PRINTF("Starting Erbium Example Server\n");

	#ifdef RF_CHANNEL
	PRINTF("RF channel: %u\n", RF_CHANNEL);
	#endif
	#ifdef IEEE802154_PANID
	PRINTF("PAN ID: 0x%04X\n", IEEE802154_PANID);
	#endif

	PRINTF("uIP buffer: %u\n", UIP_BUFSIZE);
	PRINTF("LL header: %u\n", UIP_LLH_LEN);
	PRINTF("IP+UDP header: %u\n", UIP_IPUDPH_LEN);
	PRINTF("REST max chunk: %u\n", REST_MAX_CHUNK_SIZE);

	/* Initialize the REST engine. */
	rest_init_engine();

	/* Activate the application-specific resources. */
	#if REST_RES_CHUNKS
	rest_activate_resource(&resource_chunks);
	#endif
	#if defined (PLATFORM_HAS_LEDS)
	#if REST_RES_LEDS
	rest_activate_resource(&resource_leds);
	#endif
	#if REST_RES_TOGGLE
	rest_activate_resource(&resource_toggle);
	#endif
	#endif /* PLATFORM_HAS_LEDS */
	#if REST_RES_SUM
	rest_activate_resource(&resource_sum);
	#endif
	#if REST_RES_TIMES
	rest_activate_resource(&resource_times);
	#endif

	/* Define application-specific events here. */
	while(1) {
		PROCESS_WAIT_EVENT();
		#if defined (PLATFORM_HAS_BUTTON)
		if (ev == sensors_event && data == &button_sensor) {
			PRINTF("BUTTON\n");
		}
		#endif /* PLATFORM_HAS_BUTTON */
	} /* while (1) */

	PROCESS_END();
}
