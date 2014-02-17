#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct cmd {
  uint8_t cmd;
  uint8_t status_boca1;
	uint8_t status_boca2;
	uint8_t status_boca3;
	uint8_t status_boca4;
	uint8_t status_forno;  
} cmd_t;

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
   inet_pton(AF_INET6, "FEC0::2", &addr.sin6_addr.s6_addr);
   addr.sin6_port = htons(9000);

   c.cmd = 141;

   sendto(sockfd, &c, sizeof(c), 0, (struct sockaddr*)&addr, sizeof(addr));

	 cmd_t buf;
	
	 if( (recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr*)&remaddr, &addrlen) > 0))
	 {
		 if (buf.status_boca1 == 1)
		 		printf("BOCA 1: ACESA.\n");
		 else 
		 		printf("BOCA 1: APAGADA.\n");		
		 		
		 if (buf.status_boca2 == 1)
		 		printf("BOCA 2: ACESA.\n");
		 else 
		 		printf("BOCA 2: APAGADA.\n");		
		 		
		 if (buf.status_boca3 == 1)
		 		printf("BOCA 3: ACESA.\n");
		 else 
		 		printf("BOCA 3: APAGADA.\n");		
		 		
		 if (buf.status_boca4 == 1)
		 		printf("BOCA 4: ACESA.\n");
		 else 
		 		printf("BOCA 4: APAGADA.\n");		
		 	
	 	 if (buf.status_forno == 1)
		 		printf("FORNO ACESO.\n");
		 else 
		 		printf("FORNO APAGADO.\n");		 
	 }
	
   close(sockfd);

   return 0;
}
