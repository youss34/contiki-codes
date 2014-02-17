#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"

#include "lib/random.h"
#include "sys/etimer.h"

#include "net/uip.h"
#include "net/rpl/rpl.h"
#include "net/netstack.h"

#include "dev/button-sensor.h"

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

enum commands {
	TURN_OFF, TURN_0N, GET_STATUS
};

static struct uip_udp_conn *server_conn;

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);

static void answer_receiver()
{
	if(uip_newdata()) {
		int *answer = (int *)uip_appdata;
		int status = *answer;
		printf("Status received from lamp ");
		//uip_debug_ipaddr_print(sender_addr);
		printf(" : '%d'\n", status);
	}
}

void lamp_commands(short command, uip_ipaddr_t *lamp_addr)
{
	uip_udp_packet_sendto(server_conn, &command, sizeof(short),
			lamp_addr, UIP_HTONS(UDP_CLIENT_PORT));
}

static uip_ipaddr_t * set_global_address(void)
{
	static uip_ipaddr_t ipaddr;

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

	return &ipaddr;
}

static void create_rpl_dag(uip_ipaddr_t *ipaddr)
{
	struct uip_ds6_addr *root_if;

	root_if = uip_ds6_addr_lookup(ipaddr);
	if(root_if != NULL) {
		rpl_dag_t *dag;
		dag = rpl_set_root(RPL_DEFAULT_INSTANCE,(uip_ip6addr_t *)&ipaddr);
		uip_ip6addr(ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
		rpl_set_prefix(dag, ipaddr, 64);
		PRINTF("Created a new RPL dag\n");
	}
	else {
		PRINTF("Failed to create a new RPL DAG\n");
	}
}

static void print_local_addresses()
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

PROCESS_THREAD(udp_server_process, ev, data)
{
	uip_ipaddr_t* ipaddr;
	uip_ipaddr_t lamp_addr;
	static struct etimer send_timer;
	static int flag;
	
	PROCESS_BEGIN();

	PROCESS_PAUSE();

	SENSORS_ACTIVATE(button_sensor);

	PRINTF("UDP server started\n");

	#if UIP_CONF_ROUTER
	ipaddr = set_global_address();
	create_rpl_dag(ipaddr);
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

	flag = 0;
	etimer_set(&send_timer, 10 * CLOCK_SECOND);
	while(1) {
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			answer_receiver();
		}
		else if(etimer_expired(&send_timer)) {
			puts("Sending request...");
			//uip_ip6addr(&lamp_addr, 0xaaaa, 0, 0, 0, 0, 0, 0, 2);
			uip_ip6addr(&lamp_addr, 0xfe80, 0, 0, 0, 0x0212, 0x7402, 0x0002, 0x0202);
			printf("Destiny address ");
			uip_debug_ipaddr_print(&lamp_addr);
			printf("\n");
			if(flag == 0){
				lamp_commands(GET_STATUS, &lamp_addr);
				flag = 1;
			}
			else if(flag == 1){
				lamp_commands(TURN_0N, &lamp_addr);
				flag = 2;
			}
			else if(flag == 2){
				lamp_commands(GET_STATUS, &lamp_addr);
				flag = 3;
			}
			else{
				lamp_commands(TURN_OFF, &lamp_addr);
				flag = 0;
			}
			etimer_reset(&send_timer);
		}
		else if (ev == sensors_event && data == &button_sensor) {
			PRINTF("Initiaing global repair\n");
			rpl_repair_root(RPL_DEFAULT_INSTANCE);
		}
	}

	PROCESS_END();
}
