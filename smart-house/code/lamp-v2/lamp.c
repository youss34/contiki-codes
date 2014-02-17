#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "dev/leds.h"

#include <string.h>

#define UIP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

struct cmd {
  uint8_t cmd;
  uint8_t on_off;
};

static struct uip_udp_conn *lamp_conn;

PROCESS(lamp_process, "Lamp process");
AUTOSTART_PROCESSES(&lamp_process);

static void udp_handler(void)
{
	if(uip_newdata()) {
		struct cmd *command = (struct cmd *)uip_appdata;

		PRINTF("Command %u received from ", command->cmd);
		PRINT6ADDR(&(UIP_IP_BUF->srcipaddr));
		PRINTF("\n");
		PRINTF("Port: %u\n", UIP_HTONS(UIP_IP_BUF->srcport));
		
		if(command->cmd == 101){
			unsigned char status = leds_get();
			uip_udp_packet_sendto(lamp_conn, &status, sizeof(unsigned char),
				&UIP_IP_BUF->srcipaddr, UIP_IP_BUF->srcport);
		}
		else if(command->on_off == 1){
			leds_on(LEDS_ALL);
		}
		else if(command->on_off == 2){
			leds_off(LEDS_ALL);
		}
		else{
			PRINTF("Unknown command!\n");
		}
	}
}

PROCESS_THREAD(lamp_process, ev, data)
{
	uip_ipaddr_t ipaddr;

	PROCESS_BEGIN();

	PRINTF("Lamp process started\n");

	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	lamp_conn = udp_new(NULL, 0, NULL);
	udp_bind(lamp_conn, UIP_HTONS(9000));

	while(1){
		PROCESS_YIELD();
		if(ev == tcpip_event) {
			udp_handler();
		}
	}

	PROCESS_END();
}
