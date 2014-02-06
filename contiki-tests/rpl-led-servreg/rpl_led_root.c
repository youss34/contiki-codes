#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "sys/etimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-debug.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"

#include "sys/node-id.h"

#include "simple-udp.h"
#include "servreg-hack.h"

#include "net/rpl/rpl.h"

#include <stdio.h>
#include <string.h>

#define UDP_PORT 	1234
#define MESSAGE_SERVICE_ID 	190
#define LED_SERVICE_ID 		191

#define SEND_INTERVAL 	(10 * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define RPL_ROOT 1
#define LED_ROOT 2

static struct simple_udp_connection unicast_connection;

/*---------------------------------------------------------------------------*/
PROCESS(unicast_receiver_process, "Unicast receiver example process");
AUTOSTART_PROCESSES(&unicast_receiver_process);
/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
			const uip_ipaddr_t *sender_addr,
			uint16_t sender_port,
			const uip_ipaddr_t *receiver_addr,
			uint16_t receiver_port,
			const uint8_t *data,
			uint16_t datalen)
{
	printf("Data received from ");
	uip_debug_ipaddr_print(sender_addr);
	printf(" on port %d from port %d with length %d: '%s'\n",
	receiver_port, sender_port, datalen, data);
}

static void
led_service(struct simple_udp_connection *c,
			const uip_ipaddr_t *sender_addr,
			uint16_t sender_port,
			const uip_ipaddr_t *receiver_addr,
			uint16_t receiver_port,
			const uint8_t *data,
			uint16_t datalen)
{
	unsigned short combination = *data;
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
/*---------------------------------------------------------------------------*/
static uip_ipaddr_t *
set_global_address(void)
{
	static uip_ipaddr_t ipaddr;
	int i;
	uint8_t state;

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr); //usado apenas e configuracoes automaticas de IPv6
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	printf("IPv6 addresses: ");
	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if(uip_ds6_if.addr_list[i].isused &&
			(state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
			uip_debug_ipaddr_print(&uip_ds6_if.addr_list[i].ipaddr);
			printf("\n");
		}
	}

	return &ipaddr;
}
/*---------------------------------------------------------------------------*/
static void
create_rpl_dag(uip_ipaddr_t *ipaddr)
{
	struct uip_ds6_addr *root_if;

	root_if = uip_ds6_addr_lookup(ipaddr);
	if(root_if != NULL) {
		rpl_dag_t *dag;
		uip_ipaddr_t prefix;

		rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
		dag = rpl_get_any_dag();
		uip_ip6addr(&prefix, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
		rpl_set_prefix(dag, &prefix, 64);
		PRINTF("created a new RPL dag\n");
	}
	else {
		PRINTF("failed to create a new RPL DAG\n");
	}
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_receiver_process, ev, data)
{
	uip_ipaddr_t *ipaddr;

	PROCESS_BEGIN();

	servreg_hack_init();

	ipaddr = set_global_address();

	//create_rpl_dag(ipaddr);

	if(node_id == RPL_ROOT){
		create_rpl_dag(ipaddr);
		servreg_hack_register(MESSAGE_SERVICE_ID, ipaddr);
		simple_udp_register(&unicast_connection, UDP_PORT,
								NULL, UDP_PORT, receiver);
	}
	else if(node_id == LED_ROOT){
		servreg_hack_register(LED_SERVICE_ID, ipaddr);
		simple_udp_register(&unicast_connection, UDP_PORT,
								NULL, UDP_PORT, led_service);
	}

	while(1) {
		PROCESS_WAIT_EVENT();
	}
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
