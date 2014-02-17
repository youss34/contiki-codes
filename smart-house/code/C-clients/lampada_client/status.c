#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct cmd {
  uint8_t cmd;
};

int main(int argc, char**argv)
{
   int sockfd;
   
   struct cmd c;
   
   struct sockaddr_in6 addr;
	 struct sockaddr_in6 remaddr;
	 socklen_t addrlen = sizeof(remaddr);

   sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

   memset(&addr, 0, sizeof(addr));

   addr.sin6_family = AF_INET6;
   inet_pton(AF_INET6, "aaaa::212:7402:2:202", &addr.sin6_addr.s6_addr);
   addr.sin6_port = htons(9000);

   c.cmd = 101;


   sendto(sockfd, &c, sizeof(c), 0, (struct sockaddr*)&addr, sizeof(addr));

	unsigned char status;
	
	if( (recvfrom(sockfd, &status, sizeof(unsigned char), 0, (struct sockaddr*)&remaddr, &addrlen) > 0))
	{
		printf("Mensagem recebida: %u \n", status);
	}
	
  close(sockfd);

   return 0;
}
