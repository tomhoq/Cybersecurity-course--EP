#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include "header.h"

#define DHCP_MAGIC_COOKIE 0x63825363
#define SRC_IP  "0.0.0.0"
#define SRC_PORT 68
#define DEST_IP "255.255.255.255"
#define DEST_PORT 67

int dhcp_discover(int sockfd, const char *ifname, uint32_t xid, uint8_t *mac);
int dhcp_receive(int sockfd, struct sockaddr_in *server_addr, uint32_t *yiaddr, uint32_t *server_ip, uint32_t expected_xid);
int dhcp_request(int sockfd, const char *ifname, uint32_t xid, uint32_t yiaddr, uint32_t server_ip, uint8_t *mac);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <interface>\n", argv[0]);
        return 1;
    }

    int fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("Error creating raw socket ");
        exit(1);
    }

    int hincl = 1;
    setsockopt(fd, IPPROTO_IP, IP_HDRINCL, &hincl, sizeof(hincl));

    int broadcast = 1;
    setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons(DEST_PORT);

    while (1) {
        uint32_t xid;
        generate_random_xid(&xid);
        uint8_t mac[6] = {0x16,0x4a,0xe8,0x76,0x41,0x01};
        //generate_random_mac(mac);

        if (dhcp_discover(fd, argv[1], xid, mac) != 0) break;

        uint32_t yiaddr = 0, server_ip = 0;
        if (dhcp_receive(fd, &server_addr, &yiaddr, &server_ip, xid) != 0) break;

        sleep(1);
        if (dhcp_request(fd, argv[1], xid, yiaddr, server_ip, mac) != 0) break;

        break; // Single cycle for now
    }

    close(fd);
    return 0;
}

int dhcp_discover(int sockfd, const char* ifname, uint32_t xid, uint8_t *mac) {
    char frame[65536];  // Full Ethernet frame (eth + ip + udp + dhcp)
    char *packet = frame + sizeof(struct ethhdr);  // Offset for IP+UDP+DHCP
    char pseudo[65536];

    memset(frame, 0, sizeof(frame));
    memset(pseudo, 0, sizeof(pseudo));

    struct ethhdr *eth = (struct ethhdr *)frame;
    struct iphdr *iph = (struct iphdr *)(packet);
    struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
    struct dhcphdr *dhcp = (struct dhcphdr *)(packet + sizeof(struct iphdr) + sizeof(struct udphdr));
    struct pseudo_udp_header psh;

    char source_ip[] = SRC_IP;
    char dest_ip[] = DEST_IP;

    // DHCP
    dhcp->op = 1;
    dhcp->htype = 1;
    dhcp->hlen = 6;
    dhcp->hops = 0;
    dhcp->xid = xid;
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000);
    dhcp->ciaddr = 0;
    dhcp->yiaddr = 0;
    dhcp->siaddr = 0;
    dhcp->giaddr = 0;
    memcpy(dhcp->chaddr, mac, 6);
    dhcp->magic = htonl(DHCP_MAGIC_COOKIE);

    // DHCP Options
    int index = 0;
    dhcp->options[index++] = 53; // DHCP Message Type
    dhcp->options[index++] = 1;
    dhcp->options[index++] = 1;  // DHCPDISCOVER
    dhcp->options[index++] = 255;  // End option

    int dhcp_total_len = sizeof(struct dhcphdr) + index;

    // IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + dhcp_total_len);
    iph->id = htons(54321);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP;
    iph->saddr = inet_addr(source_ip);
    iph->daddr = inet_addr(dest_ip);
    iph->check = 0;
    iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr));

    // UDP Header
    udph->source = htons(SRC_PORT);
    udph->dest = htons(DEST_PORT);
    udph->len = htons(sizeof(struct udphdr) + dhcp_total_len);
    udph->check = 0;

    // UDP Checksum
    psh.source_address = iph->saddr;
    psh.dest_address = iph->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = udph->len;

    int psize = sizeof(struct pseudo_udp_header) + ntohs(udph->len);
    memcpy(pseudo, &psh, sizeof(struct pseudo_udp_header));
    memcpy(pseudo + sizeof(struct pseudo_udp_header), udph, ntohs(udph->len));
    udph->check = checksum((unsigned short *)pseudo, psize);

    // Ethernet Header
    memset(eth->h_dest, 0xff, ETH_ALEN);         // Broadcast
    memcpy(eth->h_source, mac, ETH_ALEN);        // Source MAC
    eth->h_proto = htons(ETH_P_IP);

    // sockaddr_ll for link-layer
    struct sockaddr_ll dest;
    memset(&dest, 0, sizeof(dest));
    dest.sll_family = AF_PACKET;
    dest.sll_protocol = htons(ETH_P_IP);
    dest.sll_ifindex = if_nametoindex(ifname);
    dest.sll_halen = ETH_ALEN;
    memset(dest.sll_addr, 0xff, ETH_ALEN);  // Broadcast MAC

    // Final full frame length (eth + ip + udp + dhcp)
    int packet_len = sizeof(struct ethhdr) + sizeof(struct iphdr) + sizeof(struct udphdr) + dhcp_total_len;

    if (sendto(sockfd, frame, packet_len, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto raw ethernet");
        return -1;
    }

    printf("DHCPDISCOVER sent.\n");
    return 0;
}



int dhcp_receive(int sockfd, struct sockaddr_in *server_addr, uint32_t *yiaddr, uint32_t *server_ip, uint32_t expected_xid) {
    char buf[65536];
    struct sockaddr_ll sender;
    socklen_t sender_len = sizeof(sender);

    while (1) {
        int len = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&sender, &sender_len);
        if (len < 0) {
            perror("recvfrom");
            return -1;
        }

        struct iphdr *iph = (struct iphdr *)buf;
        if (iph->protocol != IPPROTO_UDP) continue;

        struct udphdr *udph = (struct udphdr *)(buf + iph->ihl * 4);
        if (ntohs(udph->dest) != SRC_PORT) continue;

        struct dhcphdr *dhcp = (struct dhcphdr *)(buf + iph->ihl * 4 + sizeof(struct udphdr));
        if (dhcp->op != 2 || ntohl(dhcp->xid) != expected_xid) continue; // not an offer

        uint8_t *opts = dhcp->options;
        if (opts[0] == 53 && opts[2] == 2) { // DHCP Offer
            *yiaddr = dhcp->yiaddr;
            *server_ip = iph->saddr;
            printf("Received DHCPOFFER:\n");
            printf("  Offered IP: %s\n", inet_ntoa(*(struct in_addr *)&dhcp->yiaddr));
            printf("  Transaction ID: 0x%x\n", ntohl(dhcp->xid));
            return 0;
        }
    }

    return -1;
}


int dhcp_request(int sockfd, const char* ifname, uint32_t xid, uint32_t yiaddr, uint32_t server_ip, uint8_t *mac) {
    char packet[65536], pseudo[65536];
    memset(packet, 0, sizeof(packet));
    memset(pseudo, 0, sizeof(pseudo));

    struct iphdr *iph = (struct iphdr *)packet;
    struct udphdr *udph = (struct udphdr *)(packet + sizeof(struct iphdr));
    struct dhcphdr *dhcp = (struct dhcphdr *)(packet + sizeof(struct iphdr) + sizeof(struct udphdr));
    struct pseudo_udp_header psh;

    char source_ip[] = SRC_IP;
    char dest_ip[] = DEST_IP;

    // Fill DHCP
    dhcp->op = 1;
    dhcp->htype = 1;
    dhcp->hlen = 6;
    dhcp->hops = 0;
    dhcp->xid = xid;
    dhcp->secs = 0;
    dhcp->flags = htons(0x8000); // broadcast
    dhcp->ciaddr = 0;
    dhcp->yiaddr = 0;
    dhcp->siaddr = 0;
    dhcp->giaddr = 0;
    memcpy(dhcp->chaddr, mac, 6);
    memset(dhcp->sname, 0, sizeof(dhcp->sname));
    memset(dhcp->file, 0, sizeof(dhcp->file));
    dhcp->magic = htonl(DHCP_MAGIC_COOKIE);

    int index = 0;
    dhcp->options[index++] = 53; // DHCP Message Type
    dhcp->options[index++] = 1;
    dhcp->options[index++] = 3;  // DHCPREQUEST

    dhcp->options[index++] = 50; // Requested IP Address
    dhcp->options[index++] = 4;
    memcpy(&dhcp->options[index], &yiaddr, 4);
    index += 4;

    dhcp->options[index++] = 54; // Server Identifier
    dhcp->options[index++] = 4;
    memcpy(&dhcp->options[index], &server_ip, 4);
    index += 4;

    dhcp->options[index++] = 255; // End

    // Fill IP Header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct udphdr) + sizeof(struct dhcphdr) + index);
    iph->id = htons(54322);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_UDP;
    iph->saddr = inet_addr(source_ip);
    iph->daddr = inet_addr(dest_ip);
    iph->check = 0;
    iph->check = checksum((unsigned short *)iph, sizeof(struct iphdr));

    // Fill UDP Header
    udph->source = htons(SRC_PORT);
    udph->dest = htons(DEST_PORT);
    udph->len = htons(sizeof(struct udphdr) + sizeof(struct dhcphdr) + index);
    udph->check = 0;

    psh.source_address = iph->saddr;
    psh.dest_address = iph->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_UDP;
    psh.udp_length = udph->len;

    memcpy(pseudo, &psh, sizeof(struct pseudo_udp_header));
    memcpy(pseudo + sizeof(struct pseudo_udp_header), udph, ntohs(udph->len));
    udph->check = checksum((unsigned short *)pseudo, sizeof(struct pseudo_udp_header) + ntohs(udph->len));

    // Set destination
    struct sockaddr_ll dest;
    memset(&dest, 0, sizeof(dest));
    dest.sll_family = AF_PACKET;
    dest.sll_protocol = htons(ETH_P_IP);
    dest.sll_ifindex = if_nametoindex(ifname);
    dest.sll_halen = ETH_ALEN;
    memset(dest.sll_addr, 0xff, ETH_ALEN); // Broadcast MAC

    int packet_len = ntohs(iph->tot_len);
    if (sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
        perror("sendto DHCPREQUEST");
        return -1;
    }

    printf("DHCPREQUEST sent.\n");
    return 0;
}



