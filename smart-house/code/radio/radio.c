#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"

#include <string.h>

#include "./SmartHome.h"

#define UIP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

static struct uip_udp_conn *radio_conn;
static radio_status_t radio_status;

PROCESS(radio_process, "Radio process");
AUTOSTART_PROCESSES(&radio_process);

static void udp_handler(void)
{
	if(uip_newdata()) {
		cmdf_t *command = (cmdf_t *)uip_appdata;
		cmd_t *icommand = (cmd_t *)uip_appdata;;

		PRINTF("Command %u received from ", command->id);
		PRINT6ADDR(&(UIP_IP_BUF->srcipaddr));
		PRINTF("\n");
		PRINTF("Port: %u\n", UIP_HTONS(UIP_IP_BUF->srcport));
		
		if(command->id == GET_STATUS){
			radio_status.id = RESP_GET_STATUS;
			uip_udp_packet_sendto(radio_conn, &radio_status,
				sizeof(radio_status_t), &UIP_IP_BUF->srcipaddr,
				UIP_IP_BUF->srcport);
		}
		else if(command->id == CMD_TURN){
			if(icommand->info == TURN_ON){
				radio_status.on_off = 1;
				leds_on(LEDS_ALL);
			}
			else if(icommand->info == TURN_OFF){
				radio_status.on_off = 0;
				leds_off(LEDS_ALL);
			}
		}
		else if(command->id == SET_VOLUME){
			radio_status.volume = icommand->info;
		}
		else if(command->id == GET_VOLUME){
			cmd_t c;
			c.info = radio_status.volume;
			c.id = RESP_GET_VOLUME;
			uip_udp_packet_sendto(radio_conn, &c, sizeof(cmd_t),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else if(command->id == SET_STATION){
			radio_status.station = command->info;
		}
		else if(command->id == GET_STATION){
			cmdf_t c;
			c.info = radio_status.station;
			c.id = RESP_GET_STATION;
			uip_udp_packet_sendto(radio_conn, &c, sizeof(cmdf_t),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else{
			PRINTF("Unknown command!\n");
		}
	}
}

PROCESS_THREAD(radio_process, ev, data)
{
	uip_ipaddr_t ipaddr;

	PROCESS_BEGIN();

	PRINTF("Radio process started\n");

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	radio_conn = udp_new(NULL, 0, NULL);
	udp_bind(radio_conn, UIP_HTONS(9000));

	radio_status.on_off = 0;
	radio_status.station = 94.9;
	radio_status.volume = 25;

	while(1){
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			udp_handler();
		}
	}

	PROCESS_END();
}
