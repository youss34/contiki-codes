#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "net/uip.h"
//#include "net/rpl/rpl.h"

#include "net/netstack.h"
#include "dev/button-sensor.h"
#include <stdlib.h>
#include <ctype.h>

#include <string.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define PRINT6ADDR(addr) PRINTF(" %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x ", ((u8_t *)addr)[0], ((u8_t *)addr)[1], ((u8_t *)addr)[2], ((u8_t *)addr)[3], ((u8_t *)addr)[4], ((u8_t *)addr)[5], ((u8_t *)addr)[6], ((u8_t *)addr)[7], ((u8_t *)addr)[8], ((u8_t *)addr)[9], ((u8_t *)addr)[10], ((u8_t *)addr)[11], ((u8_t *)addr)[12], ((u8_t *)addr)[13], ((u8_t *)addr)[14], ((u8_t *)addr)[15])
#define PRINTLLADDR(lladdr) PRINTF(" %02x:%02x:%02x:%02x:%02x:%02x ",(lladdr)->addr[0], (lladdr)->addr[1], (lladdr)->addr[2], (lladdr)->addr[3],(lladdr)->addr[4], (lladdr)->addr[5])
#else
#define PRINTF(...)
#define PRINT6ADDR(addr)
#define PRINTLLADDR(addr)
#endif

#define UDP_IP_BUF   ((struct uip_udpip_hdr *)&uip_buf[UIP_LLH_LEN])

#define SEND_INTERVAL        15 * CLOCK_SECOND
#define MAX_PAYLOAD_LEN        40

static struct uip_udp_conn *client_conn;
/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  char *str;

  if(uip_newdata()) {
    str = uip_appdata;
    str[uip_datalen()] = '\0';
    printf("Response from the server: '%s'\n", str);
  }
}

static void
tcpip_handler_2(void)
{
  char buf[MAX_PAYLOAD_LEN];

  if(uip_newdata()) {
    ((char *)uip_appdata)[uip_datalen()] = 0;
    PRINTF("Response 2 : '%s' from ", (char *)uip_appdata);
    PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
    PRINTF("\n");
    //uip_ipaddr_copy(&client_conn->ripaddr, &UDP_IP_BUF->srcipaddr);
    //client_conn->rport = UDP_IP_BUF->srcport;
    PRINTF("Responding activing with message: ");
    sprintf(buf, "Chupa contiki...");
    PRINTF("%s\n", buf);

    uip_udp_packet_send(client_conn, buf, strlen(buf));
    
    /* Restore server connection to allow data from any node */
    //memset(&client_conn->ripaddr, 0, sizeof(client_conn->ripaddr));
    //client_conn->rport = 0;
 }
}
/*---------------------------------------------------------------------------*/
static void
timeout_handler(void)
{
  static int seq_id;
  char buf[MAX_PAYLOAD_LEN];

  printf("Client sending to: ");
  PRINT6ADDR(&client_conn->ripaddr);
  sprintf(buf, "Hello %d from the client", ++seq_id);
  printf(" (msg: %s)\n", buf);
  uip_udp_packet_send(client_conn, buf, strlen(buf));
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
/*---------------------------------------------------------------------------*/
static void
set_connection_address(uip_ipaddr_t *ipaddr)
{
  // change this IP address depending on the node that runs the server!
  uip_ip6addr(ipaddr,0xfe80,0,0,0,0x0212,0x7402,0x0002,0x0202);
  //uip_ip6addr(ipaddr,0xaaaa,0,0,0,0,0,0,1);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer et;
  uip_ipaddr_t ipaddr;
	static int flag;
  PROCESS_BEGIN();
  PRINTF("UDP client process started\n");

  // wait 3 second, in order to have the IP addresses well configured
  etimer_set(&et, CLOCK_CONF_SECOND*3);

  // wait until the timer has expired
  PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

  print_local_addresses();
  set_connection_address(&ipaddr);

  /* new connection with remote host */
  client_conn = udp_new(&ipaddr, UIP_HTONS(3000), NULL);
  udp_bind(client_conn, UIP_HTONS(3000));
  
  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF("local/remote port %u/%u\n",
  UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));
	flag = 0;
  etimer_set(&et, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      timeout_handler();
      etimer_restart(&et);
    } else if(ev == tcpip_event && flag == 0) {
       tcpip_handler();
       flag = 1;
    } else {
		tcpip_handler_2();
		flag = 0;
	}
  }

 PROCESS_END();
}
/*---------------------------------------------------------------------------*/
