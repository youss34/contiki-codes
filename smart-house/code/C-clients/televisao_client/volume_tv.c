#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct cmd_s {
  uint8_t cmd;
  uint8_t volume;
} cmd_t;

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
   inet_pton(AF_INET6, "FEC0::2", &addr.sin6_addr.s6_addr);
   addr.sin6_port = htons(9000);

   c.cmd = 113;
  // c.volume = 20;
	 
	 uint32_t volume;
	 
	 printf("Informe o volume desejado: ");
	 scanf("%u", &volume);
	 getchar();
	 
	 c.volume = volume;
	 
   sendto(sockfd, &c, sizeof(c), 0, (struct sockaddr*)&addr, sizeof(addr));

   cmd_t buf;
	
	if( (recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr*)&remaddr, &addrlen) > 0))
	{
		printf("Recebendo confirmação..\n");
		if (buf.cmd == 212)
			printf("Volume selecionado: %u \n", buf.volume);
	}
	
  close(sockfd);

   return 0;
}
