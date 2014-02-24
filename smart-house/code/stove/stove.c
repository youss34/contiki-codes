#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"

#include <string.h>

#include "./SmartHome.h"

#define UIP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

static struct uip_udp_conn *stove_conn;
static fogao_status_t stove_status;

PROCESS(stove_process, "Stove process");
AUTOSTART_PROCESSES(&stove_process);

static void udp_handler(void)
{
	if(uip_newdata()) {
		cmd_t *command = (cmd_t *)uip_appdata;

		PRINTF("Command %u received from ", command->id);
		PRINT6ADDR(&(UIP_IP_BUF->srcipaddr));
		PRINTF("\n");
		PRINTF("Port: %u\n", UIP_HTONS(UIP_IP_BUF->srcport));
		
		if(command->id == GET_STATUS){
			stove_status.id = RESP_GET_STATUS;
			uip_udp_packet_sendto(stove_conn, &stove_status,
				sizeof(fogao_status_t), &UIP_IP_BUF->srcipaddr,
				UIP_IP_BUF->srcport);
		}
		else if(command->id == GET_TEMPERATURE){
			cmdf_t c;
			c.info = stove_status.temperature;
			c.id = RESP_GET_TEMPERATURE;
			uip_udp_packet_sendto(stove_conn, &c, sizeof(cmdf_t),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else{
			PRINTF("Unknown command!\n");
		}
	}
}

PROCESS_THREAD(stove_process, ev, data)
{
	uip_ipaddr_t ipaddr;

	PROCESS_BEGIN();

	PRINTF("Stove process started\n");

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	stove_conn = udp_new(NULL, 0, NULL);
	udp_bind(stove_conn, UIP_HTONS(9000));

	stove_status.status_boca1 = TURN_OFF;
	stove_status.status_boca2 = TURN_ON;
	stove_status.status_boca3 = TURN_ON;
	stove_status.status_boca4 = TURN_OFF;
	stove_status.status_forno = TURN_OFF;
	stove_status.temperature = 50.0;

	while(1){
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			udp_handler();
		}
	}

	PROCESS_END();
}
