#include "contiki.h"

#include "net/uip.h"
#include "net/uip-ds6.h"
#include "net/uip-udp-packet.h"

#include "sys/node-id.h"

#include "dev/leds.h"

#ifdef WITH_COMPOWER
#include "powertrace.h"
#endif

#include <stdio.h>
#include <string.h>

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UIP_IP_BUF   ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define UDP_EXAMPLE_ID  190

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_TIME			(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30

static struct uip_udp_conn *client_conn;
static uip_ipaddr_t server_ipaddr;

PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);

int get_status();
void turn_off();
void turn_on();

static void command_receiver()
{
	if(uip_newdata()) {
		short *recv = (short*)uip_appdata;
		short command = *recv;
		if(command == 2){
			int status = get_status();
			uip_udp_packet_sendto(client_conn, &status, sizeof(int),
					&server_ipaddr, UIP_HTONS(UDP_SERVER_PORT));
		}
		else if(command == 1){
			turn_on();
		}
		else if(command == 0){
			turn_off();
		}
	}
}

int get_status()
{
	unsigned char status = leds_get();
	return (int)status;
}

void turn_on()
{
	leds_on(LEDS_ALL);
}

void turn_off()
{
	leds_off(LEDS_ALL);
}

static void print_local_addresses()
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

static void set_global_address()
{
	uip_ipaddr_t ipaddr;

	if(node_id == 2){
		uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 2);
		//uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
		//uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
		uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
	}
	else{
		uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
		uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
		uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
	}

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

PROCESS_THREAD(udp_client_process, ev, data)
{
	#if WITH_COMPOWER
	static int print = 0;
	#endif

	PROCESS_BEGIN();

	PROCESS_PAUSE();

	set_global_address();

	PRINTF("UDP client process started\n");

	print_local_addresses();

	if(node_id == 2){
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
		
		//etimer_set(&periodic, SEND_INTERVAL);
		while(1) {
			PROCESS_YIELD();
			if(ev == tcpip_event) {
				//tcpip_handler();
				puts("Command received...");
				command_receiver();
			}

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
