#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/uip.h"
#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define UDP_EXAMPLE_ID  190

static struct uip_udp_conn *server_conn;

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/

static void
led_service(unsigned short combination)
{
	leds_off(LEDS_ALL);
	if(combination == 1){
		leds_on(LEDS_GREEN);
	}
	else if(combination == 2){
		leds_on(LEDS_BLUE);
	}
	else if(combination == 3){
		leds_on(LEDS_RED);
	}
	else if(combination == 4){
		leds_on(LEDS_GREEN);
		leds_on(LEDS_BLUE);
	}
	else if(combination == 5){
		leds_on(LEDS_GREEN);
		leds_on(LEDS_RED);
	}
	else if(combination == 6){
		leds_on(LEDS_BLUE);
		leds_on(LEDS_RED);
	}
	else if(combination == 7){
		leds_on(LEDS_ALL);
	}
}

static void
tcpip_handler(void)
{
	unsigned short *appdata, combination;

	if(uip_newdata()) {
		appdata = (unsigned short *)uip_appdata;
		combination = *appdata;
		PRINTF("Combination recv '%d' from ", combination);
		PRINTF("%d",
		UIP_IP_BUF->srcipaddr.u8[sizeof(UIP_IP_BUF->srcipaddr.u8) - 1]);
		PRINTF("\n");

		led_service(combination);

		#if SERVER_REPLY
		PRINTF("Sending reply\n");
		uip_ipaddr_copy(&server_conn->ripaddr, &UIP_IP_BUF->srcipaddr);
		uip_udp_packet_send(server_conn, "Done", sizeof("Done"));
		uip_create_unspecified(&server_conn->ripaddr);
		#endif
	}
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
	int i;
	uint8_t state;

	PRINTF("Server IPv6 addresses: ");
	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if(state == ADDR_TENTATIVE || state == ADDR_PREFERRED) {
			PRINT6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
			PRINTF("\n");
			/* hack to make address "final" */
			if (state == ADDR_TENTATIVE) {
				uip_ds6_if.addr_list[i].state = ADDR_PREFERRED;
			}
		}
	}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data)
{
	uip_ipaddr_t ipaddr;
	struct uip_ds6_addr *root_if;

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	SENSORS_ACTIVATE(button_sensor);

	PRINTF("UDP server started\n");

	#if UIP_CONF_ROUTER
	
	#if 0
	/* Mode 1 - 64 bits inline */
	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
	#elif 1
	/* Mode 2 - 16 bits inline */
	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
	#else
	/* Mode 3 - derived from link local (MAC) address */
	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	#endif

	uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
	root_if = uip_ds6_addr_lookup(&ipaddr);
	if(root_if != NULL) {
		rpl_dag_t *dag;
		dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
		uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
		rpl_set_prefix(dag, &ipaddr, 64);
		PRINTF("created a new RPL dag\n");
	}
	else {
		PRINTF("failed to create a new RPL DAG\n");
	}
	#endif /* UIP_CONF_ROUTER */

	print_local_addresses();

	/* The data sink runs with a 100% duty cycle in order to ensure high 
	packet reception rates. */
	NETSTACK_MAC.off(1);

	server_conn = udp_new(NULL, UIP_HTONS(UDP_CLIENT_PORT), NULL);
	if(server_conn == NULL) {
		PRINTF("No UDP connection available, exiting the process!\n");
		PROCESS_EXIT();
	}
	udp_bind(server_conn, UIP_HTONS(UDP_SERVER_PORT));

	PRINTF("Created a server connection with remote address ");
	PRINT6ADDR(&server_conn->ripaddr);
	PRINTF(" local/remote port %u/%u\n", UIP_HTONS(server_conn->lport),
	UIP_HTONS(server_conn->rport));

	while(1) {
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			tcpip_handler();
		}
		else if (ev == sensors_event && data == &button_sensor) {
			PRINTF("Initiaing global repair\n");
			rpl_repair_root(RPL_DEFAULT_INSTANCE);
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
