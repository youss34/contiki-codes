#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"

#include <string.h>

#include "./SmartHome.h"

#define UIP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

static struct uip_udp_conn *television_conn;
static tv_status_t tv_status;

PROCESS(television_process, "Television process");
AUTOSTART_PROCESSES(&television_process);

/**
 * Function which is used to process commands sent by clients.
 */
static void udp_handler(void)
{
	if(uip_newdata()) {
		cmd_t *command = (cmd_t *)uip_appdata;

		PRINTF("Command %u received from ", command->id);
		PRINT6ADDR(&(UIP_IP_BUF->srcipaddr));
		PRINTF("\n");
		PRINTF("Port: %u\n", UIP_HTONS(UIP_IP_BUF->srcport));
		
		if(command->id == GET_STATUS){
			tv_status.id = RESP_GET_STATUS;
			uip_udp_packet_sendto(television_conn, &tv_status,
				sizeof(tv_status_t), &UIP_IP_BUF->srcipaddr,
				UIP_IP_BUF->srcport);
		}
		else if(command->id == CMD_TURN){
			if(command->info == TURN_ON){
				tv_status.on_off = TURN_ON;
				leds_on(LEDS_ALL);
			}
			else if(command->info == TURN_OFF){
				tv_status.on_off = TURN_OFF;
				leds_off(LEDS_ALL);
			}
		}
		else if(command->id == SET_VOLUME){
			tv_status.volume = command->info;
		}
		else if(command->id == GET_VOLUME){
			cmd_t c;
			c.info = tv_status.volume;
			c.id = RESP_GET_VOLUME;
			uip_udp_packet_sendto(television_conn, &c, sizeof(cmd_t),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else if(command->id == SET_CHANNEL){
			tv_status.channel = command->info;
		}
		else if(command->id == GET_CHANNEL){
			cmd_t c;
			c.info = tv_status.channel;
			c.id = RESP_GET_CHANNEL;
			uip_udp_packet_sendto(television_conn, &c, sizeof(cmd_t),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else{
			PRINTF("Unknown command!\n");
		}
	}
}

/**
 * Main process thread.
 */
PROCESS_THREAD(television_process, ev, data)
{
	uip_ipaddr_t ipaddr;

	PROCESS_BEGIN();

	PRINTF("Television process started\n");

	/* The piece of code below sets a IPv6 address automatically for a node. */
	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	television_conn = udp_new(NULL, 0, NULL);
	udp_bind(television_conn, UIP_HTONS(9000));

	/* Initializes TV's status. It can be changed whenever is necessary. */
	tv_status.id = 0;
	tv_status.on_off = TURN_OFF;
	tv_status.channel = 14;
	tv_status.volume = 25;

	/* Wait for a client command. */
	while(1){
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			udp_handler();
		}
	}

	PROCESS_END();
}
