#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"
#include "sys/ctimer.h"
#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif
#include <stdio.h>
#include <string.h>

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#define START_INTERVAL	(15 * CLOCK_SECOND)
#define SEND_INTERVAL	(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(lamp_process, "Lamp process");
AUTOSTART_PROCESSES(&lamp_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
	char *str;

	if(uip_newdata()) {
		str = uip_appdata;
		str[uip_datalen()] = '\0';
		printf("DATA recv '%s'\n", str);
	}
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
	unsigned short *data = (unsigned short *)ptr;
	unsigned short combination = *data;

	PRINTF("DATA send to %d 'Combination %d'\n",
	server_ipaddr.u8[sizeof(server_ipaddr.u8) - 1], combination);

	uip_udp_packet_sendto(client_conn, &combination, sizeof(unsigned short),
	&server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
	int i;
	uint8_t state;

	PRINTF("Client IPv6 addresses: ");
	for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
		state = uip_ds6_if.addr_list[i].state;
		if(uip_ds6_if.addr_list[i].isused &&
		(state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
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
static void
set_global_address(void)
{
	uip_ipaddr_t ipaddr;

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	#if 0
	/* Mode 1 - 64 bits inline */
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
	#elif 1
	/* Mode 2 - 16 bits inline */
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0, 0x00ff, 0xfe00, 1);
	#else
	/* Mode 3 - derived from server link-local (MAC) address */
	uip_ip6addr(&server_ipaddr, 0xaaaa, 0, 0, 0, 0x0250, 0xc2ff, 0xfea8, 0xcd1a); //redbee-econotag
	#endif
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(lamp_process, ev, data)
{
	static struct etimer periodic;
	static struct ctimer backoff_timer;
	static unsigned short combination = 0;
	#if WITH_COMPOWER
	static int print = 0;
	#endif

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	set_global_address();

	PRINTF("UDP client process started\n");

	print_local_addresses();

	/* new connection with remote host */
	client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL); 
	if(client_conn == NULL) {
		PRINTF("No UDP connection available, exiting the process!\n");
		PROCESS_EXIT();
	}
	udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT)); 

	PRINTF("Created a connection with the server ");
	PRINT6ADDR(&client_conn->ripaddr);
	PRINTF(" local/remote port %u/%u\n",
	UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

	#if WITH_COMPOWER
	powertrace_sniff(POWERTRACE_ON);
	#endif

	etimer_set(&periodic, SEND_INTERVAL);
	while(1) {
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			tcpip_handler();
		}

		if(etimer_expired(&periodic)) {
			etimer_reset(&periodic);
			combination = (random_rand() % 7) + 1;
			ctimer_set(&backoff_timer, SEND_TIME, send_packet, &combination);

			#if WITH_COMPOWER
			if (print == 0) {
				powertrace_print("#P");
			}
			if (++print == 3) {
				print = 0;
			}
			#endif
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
