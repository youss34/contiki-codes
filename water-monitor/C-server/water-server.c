#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "water-monitor.h"

int main(int argc, char**argv)
{
	puts("Initializing......");
	int sockfd, n, i;
	struct sockaddr_in6 servaddr, cliaddr;
	socklen_t len = sizeof(cliaddr);
	packet_t packet;
	ack_t ack;

	sockfd = socket(AF_INET6, SOCK_DGRAM, 0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_port = htons(9000);
	bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	puts("Waiting for datas...");
	while(1){
		n = recvfrom(sockfd, &packet, sizeof(packet_t), 0, (struct sockaddr *)&cliaddr, &len);
		if(n > 0){
			puts("Data received!!!");
			for(i = 0; i < 8; i++){
				printf("d1 = %5u | ", packet.vet[i].d1);
				printf("d2 = %5u | ", packet.vet[i].d2);
				printf("d3 = %5u | ", packet.vet[i].d3);
				printf("d4 = %5u\n", packet.vet[i].d4);
				puts("-----------+------------+------------+------------");
			}
			ack.ack = 1;
			ack.serial = packet.serial;
			printf("\nSending ack with serial equals %u\n", ack.serial);
			sendto(sockfd ,&ack, sizeof(ack_t), 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
		}
	}

	close(sockfd);
}
