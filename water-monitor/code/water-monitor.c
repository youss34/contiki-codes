#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "sys/etimer.h"
#include "dev/light-sensor.h"
#include "dev/sht11-sensor.h"

#include <string.h>

#include "./water-monitor.h"

#define UIP_IP_BUF ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_PRINT
#include "net/uip-debug.h"

static struct uip_udp_conn *monitor_conn;
static uip_ipaddr_t serv_ipaddr;
static packet_t p1;
static packet_t p2;
static packet_t *p_send;
static packet_t *p_collect;
static uint16_t serial;
static uint8_t received;

PROCESS(monitor_process, "Water monitor process");
AUTOSTART_PROCESSES(&monitor_process);

/**
 * Function which is used to process received ack packets.
 */
static void udp_handler(void)
{
	if(uip_newdata()) {
		ack_t *ack = (ack_t *)uip_appdata;
		/* If the received serial number is equal to the current serial
		 * number then old data are cleared and the current serial
		 * number is incremented. */
		if(ack->serial == serial){
			p_send = p_collect;
			received = OK;
			serial++;
		}
	}
}

static void send_data(void)
{
	p_send->serial = serial;
	uip_udp_packet_sendto(monitor_conn, p_send, sizeof(packet_t),
			&serv_ipaddr, UIP_HTONS(9000));
}

/**
 * Main process thread.
 */
PROCESS_THREAD(monitor_process, ev, data)
{
	uip_ipaddr_t ipaddr;
	static struct etimer collect;
	static struct etimer receive;
	static uint16_t light1;
	static uint16_t light2;
	static uint16_t temperature;
	static uint16_t humidity;
	static uint8_t read;

	PROCESS_BEGIN();

	PRINTF("Water monitor process started\n");

	/* The piece of code below sets a IPv6 address automatically for a node. */
	uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 0);
	uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
	uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);

	monitor_conn = udp_new(NULL, 0, NULL);
	udp_bind(monitor_conn, UIP_HTONS(9000));

	/* Sets server IPv6 address. Data are sent to that address. */
	uip_ip6addr(&serv_ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
	serial = 1;
	read = 0;
	received = FAIL;
	p_send = p_collect = &p1;
	/* Initializes collect timer. */
	etimer_set(&collect, COLLECT_PERIOD * CLOCK_SECOND);
	while(1){
		PROCESS_YIELD();
		/* Wait for server's ACK. */
		if(ev == tcpip_event) {
			udp_handler();
		}
		/* Collects sensors' values. */
		else if(ev == PROCESS_EVENT_TIMER && data == &collect) {
			SENSORS_ACTIVATE(light_sensor);
			SENSORS_ACTIVATE(sht11_sensor);

			light1 = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
			light2 = light_sensor.value(LIGHT_SENSOR_TOTAL_SOLAR);
			temperature = sht11_sensor.value(SHT11_SENSOR_TEMP);
			humidity = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);

			SENSORS_DEACTIVATE(light_sensor);
			SENSORS_DEACTIVATE(sht11_sensor);

			p_collect->vet[read].d1 = light1;
			p_collect->vet[read].d2 = light2;
			p_collect->vet[read].d3 = temperature;
			p_collect->vet[read].d4 = humidity;

			printf("Read %u data...\n", (read + 1));
			/*printf("light1: %u | ", light1);
			printf("light2: %u | ", light2);
			printf("temp  : %u | ", temperature);
			printf("humi  : %u\n", humidity);*/

			read++;
			if(read == 8){
				p_collect = (p_collect == &p1) ? &p2 : &p1;
				read = 0;
				send_data();
				/* Initializes timer which waits a ACK packet. */
				etimer_set(&receive, ACK_WAIT_TIME * CLOCK_SECOND);
			}

			etimer_reset(&collect);
		}
		/* Verifies if a ACK packet was received. If it does not then data are
		 * sent again. */
		else if(ev == PROCESS_EVENT_TIMER && data == &receive){
			if(received == FAIL){
				send_data();
				etimer_reset(&receive);
			}
			else{
				received = FAIL;
				etimer_stop(&receive);
			}
		}
	}

	PROCESS_END();
}
