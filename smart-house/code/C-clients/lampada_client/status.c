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
	 struct sockaddr_in6 remaddr;
	 socklen_t addrlen = sizeof(remaddr);

   sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

   memset(&addr, 0, sizeof(addr));

   addr.sin6_family = AF_INET6;
   inet_pton(AF_INET6, "aaaa::212:7402:2:202", &addr.sin6_addr.s6_addr);
   addr.sin6_port = htons(9000);

   c.id = GET_STATUS;

   sendto(sockfd, &c, sizeof(c), 0, (struct sockaddr*)&addr, sizeof(addr));

	if( (recvfrom(sockfd, &c, sizeof(cmd_t), 0, (struct sockaddr*)&remaddr, &addrlen) > 0))
	{
		printf("Mensagem recebida: %u \n", c.info);
	}
	
  close(sockfd);

   return 0;
}
