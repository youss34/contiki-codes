#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../SmartHome.h"

int main(int argc, char**argv)
{
   int sockfd;
   cmd_t c;
   struct sockaddr_in6 addr;

   sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

   memset(&addr, 0, sizeof(addr));

   addr.sin6_family = AF_INET6;
   inet_pton(AF_INET6, "aaaa::212:7406:6:606", &addr.sin6_addr.s6_addr);
   addr.sin6_port = htons(9000);

   c.id = CMD_TURN;
   c.info = TURN_OFF;

   sendto(sockfd, &c, sizeof(c), 0, (struct sockaddr*)&addr, sizeof(addr));

   close(sockfd);

   return 0;
}
