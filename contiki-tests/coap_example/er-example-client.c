#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "contiki-net.h"

#include "dev/button-sensor.h"

#if WITH_COAP == 3
#include "er-coap-03-engine.h"
#elif WITH_COAP == 6
#include "er-coap-06-engine.h"
#elif WITH_COAP == 7
#include "er-coap-07-engine.h"
#elif WITH_COAP == 12
#include "er-coap-12-engine.h"
#elif WITH_COAP == 13
#include "er-coap-13-engine.h"
#else
#error "CoAP version defined by WITH_COAP not implemented"
#endif


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

/* TODO: This server address is hard-coded for Cooja. */
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xaaaa, 0, 0, 0, 0x0212, 0x7402, 0x0002, 0x0202) /* cooja2 */

#define LOCAL_PORT      UIP_HTONS(COAP_DEFAULT_PORT+1)
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)

#define TOGGLE_INTERVAL 10

PROCESS(coap_client_example, "COAP Client Example");
AUTOSTART_PROCESSES(&coap_client_example);

uip_ipaddr_t server_ipaddr;
static struct etimer et;

/* Example URIs that can be queried. */
#define NUMBER_OF_URLS 3

char* service_urls[NUMBER_OF_URLS] = {"/actuators/toggle", "/actuators/leds",
										"/services/sum"};
#if PLATFORM_HAS_BUTTON
static int uri_switch = 0;
#endif

void client_chunk_handler(void *response)
{
	const uint8_t *chunk;
	coap_get_payload(response, &chunk);
	printf("|%s\n", (char *)chunk);
}

PROCESS_THREAD(coap_client_example, ev, data)
{
	PROCESS_BEGIN();
	static coap_packet_t request[1];
	static int num1 = 1, num2 = 2;
	SERVER_NODE(&server_ipaddr);

	/* receives all CoAP messages */
	coap_receiver_init();

	etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);

	#if PLATFORM_HAS_BUTTON
	SENSORS_ACTIVATE(button_sensor);
	printf("Press a button to request %s\n", service_urls[uri_switch]);
	#endif

	while(1) {
		PROCESS_YIELD();

		if (etimer_expired(&et)) {
			printf("--Toggle timer--\n");

			/* prepare request, TID is set by COAP_BLOCKING_REQUEST() */
			coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0 );
			coap_set_header_uri_path(request, service_urls[0]);

			const char msg[] = "Toggle!";
			coap_set_payload(request, (uint8_t *)msg, sizeof(msg)-1);

			PRINT6ADDR(&server_ipaddr);
			PRINTF(" : %u\n", UIP_HTONS(REMOTE_PORT));

			COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
										client_chunk_handler);

			printf("\n--Done--\n");

			etimer_reset(&et);

		#if PLATFORM_HAS_BUTTON
		}
		else if (ev == sensors_event && data == &button_sensor) {

			if(uri_switch > 3){
				if(uri_switch == 4){
					char url[REST_MAX_CHUNK_SIZE];
					snprintf(url, REST_MAX_CHUNK_SIZE, "num1=%d;num2=%d",
									num1, num2);
					coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
					printf("URL = %s\n", url);
					coap_set_header_uri_path(request, service_urls[2]);
					coap_set_header_uri_query(request, url);
					num1++;
					num2 = num1 + 2;
					uri_switch++;
				}
				else{
					coap_init_message(request, COAP_TYPE_CON, COAP_GET, 0);
					coap_set_header_uri_path(request, service_urls[2]);
					uri_switch = 0;
				}
			}
			else{
				char url[17];
				if(uri_switch == 0){
					strcpy(url, "color=g&mode=on");
					url[15] = '\0';
					uri_switch++;
				}
				else if(uri_switch == 1){
					strcpy(url, "color=g&mode=off");
					url[16] = '\0';
					uri_switch++;
				}
				else if(uri_switch == 2){
					strcpy(url, "color=b&mode=on");
					url[15] = '\0';
					uri_switch++;
				}
				else if(uri_switch == 3){
					strcpy(url, "color=b&mode=off");
					url[16] = '\0';
					uri_switch++;
				}
				coap_init_message(request, COAP_TYPE_CON, COAP_POST, 0);
				printf("URL = %s\n", url);
				coap_set_header_uri_path(request, service_urls[1]);
				coap_set_header_uri_query(request, url);
			}

			PRINT6ADDR(&server_ipaddr);
			PRINTF(" : %u\n", UIP_HTONS(REMOTE_PORT));

			COAP_BLOCKING_REQUEST(&server_ipaddr, REMOTE_PORT, request,
										client_chunk_handler);

			printf("\n--Done--\n");
			#endif
		}
	}

	PROCESS_END();
}
