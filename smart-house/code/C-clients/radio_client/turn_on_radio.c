#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct cmd {
  uint8_t cmd;
  uint8_t on_off;
};

int main(int argc, char**argv)
{
   int sockfd;
   struct cmd c;
   struct sockaddr_in6 addr;

   sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

   memset(&addr, 0, sizeof(addr));

   addr.sin6_family = AF_INET6;
   inet_pton(AF_INET6, "FEC0::2", &addr.sin6_addr.s6_addr);
   addr.sin6_port = htons(9000);

   c.cmd = 122;
   c.on_off = 10;

   sendto(sockfd, &c, sizeof(c), 0, (struct sockaddr*)&addr, sizeof(addr));

   close(sockfd);

   return 0;
}
