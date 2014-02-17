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

#define MAX_PAYLOAD_LEN 120

static struct uip_udp_conn *server_conn;

PROCESS(udp_server_process, "UDP server process");
AUTOSTART_PROCESSES(&udp_server_process);
/*---------------------------------------------------------------------------*/
static void
tcpip_handler(void)
{
  static int seq_id;
  char buf[MAX_PAYLOAD_LEN];

  if(uip_newdata()) {
    ((char *)uip_appdata)[uip_datalen()] = 0;
    PRINTF("Server received: '%s' from ", (char *)uip_appdata);
    PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
    PRINTF("\n");
    uip_ipaddr_copy(&server_conn->ripaddr, &UDP_IP_BUF->srcipaddr);
    server_conn->rport = UDP_IP_BUF->srcport;
    PRINTF("Responding with message: ");
    sprintf(buf, "Hello from the server! (%d)", ++seq_id);
    PRINTF("%s\n", buf);

    uip_udp_packet_send(server_conn, buf, strlen(buf));
    
    /* Restore server connection to allow data from any node */
    //memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
    //server_conn->rport = 0;
 }
}

static void
tcpip_handler_2(void)
{
  if(uip_newdata()) {
    ((char *)uip_appdata)[uip_datalen()] = 0;
    PRINTF("Server received 2: '%s' from ", (char *)uip_appdata);
    PRINT6ADDR(&UDP_IP_BUF->srcipaddr);
    PRINTF("\n");

    /* Restore server connection to allow data from any node */
    memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
    server_conn->rport = 0;
 }
}

static void
activing(void)
{
  char buf[MAX_PAYLOAD_LEN];

    //uip_ipaddr_copy(&server_conn->ripaddr, &UDP_IP_BUF->srcipaddr);
    //server_conn->rport = UDP_IP_BUF->srcport;
    
    PRINTF("Sending secondary message: \n");
    sprintf(buf, "Second message");
    uip_udp_packet_send(server_conn, buf, strlen(buf));
    /* Restore server connection to allow data from any node */
    memset(&server_conn->ripaddr, 0, sizeof(server_conn->ripaddr));
    server_conn->rport = 0;
}
/*---------------------------------------------------------------------------*/
static void
print_local_addresses(void)
{
  int i;
  uint8_t state;

  PRINTF("Server IPv6 addresses: ");
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
PROCESS_THREAD(udp_server_process, ev, data)
{
   static struct etimer timer;
   static int flag;

   PROCESS_BEGIN();
   PRINTF("UDP server started\n");

   // wait 3 second, in order to have the IP addresses well configured
   etimer_set(&timer, CLOCK_CONF_SECOND*3);

   // wait until the timer has expired
   PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
	
   uip_ipaddr_t ipaddr;

   uip_ip6addr(&ipaddr, 0xaaaa, 0, 0, 0, 0, 0, 0, 1);
   uip_ds6_addr_add(&ipaddr, 0, ADDR_MANUAL);
  
   print_local_addresses();

   // set NULL and 0 as IP address and port to accept packet from any node and any srcport.
   server_conn = udp_new(NULL, UIP_HTONS(0), NULL);
   udp_bind(server_conn, UIP_HTONS(3000));

   PRINTF("Server listening on UDP port %u\n", UIP_HTONS(server_conn->lport));
	flag = 0;
   while(1) {
     PROCESS_YIELD();
     if(ev == tcpip_event && flag == 0) {
       tcpip_handler();
       activing();
       flag = 1;
     }
     else{
		tcpip_handler_2();
       flag = 0;
	 }
   }

   PROCESS_END();
 }
/*---------------------------------------------------------------------------*/
