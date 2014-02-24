#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"

#include <string.h>

#include "./SmartHome.h"

#define UIP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

static struct uip_udp_conn *thermostat_conn;
static termostato_status_t thermostat_status;

PROCESS(thermostat_process, "Thermostat process");
AUTOSTART_PROCESSES(&thermostat_process);

static void udp_handler(void)
{
	if(uip_newdata()) {
		cmdf_t *command = (cmdf_t *)uip_appdata;
		cmd_t *icommand = (cmd_t *)uip_appdata;

		PRINTF("Command %u received from ", command->id);
		PRINT6ADDR(&(UIP_IP_BUF->srcipaddr));
		PRINTF("\n");
		PRINTF("Port: %u\n", UIP_HTONS(UIP_IP_BUF->srcport));
		
		if(command->id == GET_STATUS){
			thermostat_status.id = RESP_GET_STATUS;
			uip_udp_packet_sendto(thermostat_conn, &thermostat_status,
				sizeof(termostato_status_t), &UIP_IP_BUF->srcipaddr,
				UIP_IP_BUF->srcport);
		}
		else if(command->id == CMD_TURN){
			if(icommand->info == TURN_ON){
				thermostat_status.on_off = TURN_ON;
				leds_on(LEDS_ALL);
			}
			else if(icommand->info == TURN_OFF){
				thermostat_status.on_off = TURN_OFF;
				leds_off(LEDS_ALL);
			}
		}
		else if(command->id == SET_TEMPERATURE){
			thermostat_status.temperature = command->info;
		}
		else if(command->id == GET_TEMPERATURE){
			cmdf_t c;
			c.info = thermostat_status.temperature;
			c.id = RESP_GET_TEMPERATURE;
			uip_udp_packet_sendto(thermostat_conn, &c, sizeof(cmdf_t),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else{
			PRINTF("Unknown command!\n");
		}
	}
}

PROCESS_THREAD(thermostat_process, ev, data)
{
	uip_ipaddr_t ipaddr;

	PROCESS_BEGIN();

	PRINTF("thermostat process started\n");

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	thermostat_conn = udp_new(NULL, 0, NULL);
	udp_bind(thermostat_conn, UIP_HTONS(9000));

	thermostat_status.on_off = TURN_OFF;
	thermostat_status.temperature = 27.50;

	while(1){
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			udp_handler();
		}
	}

	PROCESS_END();
}
