/*
 * rawip_example.c
 *
 *  Created on: May 4, 2016
 *      Author: jiaziyi
 */


#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include "header.h"

#define SRC_IP  "192.168.1.111" //set your source ip here. It can be a fake one
#define SRC_PORT 54321 //set the source port here. It can be a fake one

#define DEST_IP "129.104.89.108" //set your destination ip here
#define DEST_PORT 5432 //set the destination port here
#define TEST_STRING "test data" //a test string as packet payload

struct sockaddr_in source,dest;
FILE *logfile;

int main(int argc, char *argv[])
{
	char source_ip[] = SRC_IP;
	char dest_ip[] = DEST_IP;


	int fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    int hincl = 1;                  /* 1 = on, 0 = off */
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

	if(fd < 0)
	{
		perror("Error creating raw socket ");
		exit(1);
	}

	char packet[65536], *data, pseudo[65536];
	char data_string[] = TEST_STRING;
	memset(packet, 0, 65536);

	//IP header pointer
	struct iphdr *iph = (struct iphdr *)packet;

	//UDP header pointer
	struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
	struct pseudo_udp_header psh; //pseudo header

	//data section pointer
	data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);

	//fill the data section
	strncpy(data, data_string, strlen(data_string));

	//fill the IP header here
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + strlen(data_string);
	printf("tot_len: %d\n", iph->tot_len);
	iph->id = htons(54321); //Id of this packet
	iph->frag_off = 0;
	iph->ttl = 255; //Time to live
	iph->protocol = IPPROTO_UDP; //UDP protocol
	iph->saddr = inet_addr(source_ip); //Source IP address
	iph->daddr = inet_addr(dest_ip); //Destination IP address
	iph->check = 0;
    iph->check = checksum((unsigned short *)packet, sizeof(struct iphdr)); //IP checksum
    
    //print_ip_header(packet, iph->tot_len); //print the packet

    //printf("IP checksum: %x\n", iph->check);
	//fill the UDP header
	psh.source_address = inet_addr(source_ip); //Source IP
	psh.dest_address = inet_addr(dest_ip); //Destination IP
	psh.placeholder = 0;
	psh.protocol = IPPROTO_UDP; //UDP protocol
	psh.udp_length = htons(sizeof(struct udphdr) + strlen(data_string)); //UDP length

	udph->source = htons(SRC_PORT); //Source port
	udph->dest = htons(DEST_PORT); //Destination port
	udph->len = htons(sizeof(struct udphdr) + strlen(data_string)); //UDP length
	memcpy(pseudo, (char *)&psh, sizeof(struct pseudo_udp_header)); //pseudo header
	memcpy(pseudo + sizeof(struct pseudo_udp_header), udph, sizeof(struct udphdr) + strlen(data_string)); //UDP header + data
    udph->check = 0;
	udph->check = checksum((unsigned short *)pseudo, sizeof(struct pseudo_udp_header) + sizeof(struct udphdr) + strlen(data_string)); //UDP checksum
	//printf("UDP checksum: %x\n", udph->check);
	//send the packet
	dest.sin_family = AF_INET; //IPv4
	dest.sin_port = htons(DEST_PORT); //Destination port
	dest.sin_addr.s_addr = inet_addr(dest_ip); //Destination IP address
	
	if(sendto(fd, packet, iph->tot_len, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0)
	{
		perror("Send failed");
		exit(1);
	}
	return 0;

}
