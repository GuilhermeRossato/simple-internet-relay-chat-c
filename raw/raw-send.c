#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include "raw.h"

char binary_origin_mac[6];
char bcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
char dst_mac[6]   = {0x00, 0x00, 0x00, 0x22, 0x22, 0x22};
char src_mac[6]   = {0x00, 0x00, 0x00, 0x33, 0x33, 0x33};

union eth_buffer buffer_u;

uint32_t ipchksum(uint8_t *packet)
{
	uint32_t sum=0;
	uint16_t i;

	for(i = 0; i < 20; i += 2)
		sum += ((uint32_t)packet[i] << 8) | (uint32_t)packet[i + 1];
	while (sum & 0xffff0000)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum;
}

int _irc_send_udp_packet(
	char * interface_name,
	int origin_port,
	int target_port,
	uint8_t * binary_target_mac,
	uint8_t * binary_target_ip,
	char * message
) {

	struct ifreq if_idx, if_mac, ifopts, if_ip;
	struct sockaddr_ll socket_address;
	int sockfd;
	uint8_t * msg = message;
    int message_buffer_size = strlen(message)+1;

	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd == -1) {
		perror("socket error\n");
	}

	int op_result;

	printf("Setting interface: ");
	strncpy(ifopts.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	printf("%d\n", op_result);

	printf("Setting promiscuous mode: ");
	ifopts.ifr_flags |= IFF_PROMISC;
	op_result = ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	printf("%d\n", op_result);

	printf("Getting the index of the interface: ");
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFINDEX, &if_idx);
	printf("%d\n", op_result);
	if (op_result < 0) {
		return 0;
	}
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;

	printf("Getting the MAC of the interface: ");
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFHWADDR, &if_mac);
	printf("%d\n", op_result);
	if (op_result < 0) {
		return printf("could not get mac from interface (SIOCGIFHWADDR)\n");
	}
	memcpy(binary_origin_mac, if_mac.ifr_hwaddr.sa_data, 6);


	printf("Getting the IP of the interface: ");
	memset(&if_ip, 0, sizeof(struct ifreq));
	strncpy(if_ip.ifr_name, interface_name, IFNAMSIZ-1);
 	op_result = ioctl(sockfd, SIOCGIFADDR, &if_ip);
 	printf("%d\n", op_result);

 	char ipv4_string[64];
 	snprintf(ipv4_string, 64, "%s", inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));

	printf("Source IP: %s\n", ipv4_string);

	memcpy(buffer_u.cooked_data.ethernet.dst_addr, bcast_mac, 6);
	memcpy(buffer_u.cooked_data.ethernet.src_addr, src_mac, 6);
	buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

	buffer_u.cooked_data.payload.ip.ver = 0x45;
	buffer_u.cooked_data.payload.ip.tos = 0x00;
	buffer_u.cooked_data.payload.ip.len = htons(sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.ip.id = htons(0x00);
	buffer_u.cooked_data.payload.ip.off = htons(0x00);
	buffer_u.cooked_data.payload.ip.ttl = 50;
	buffer_u.cooked_data.payload.ip.proto = 17; //0xff;
	buffer_u.cooked_data.payload.ip.sum = htons(0x0000);

	buffer_u.cooked_data.payload.ip.src[0] = 192;
	buffer_u.cooked_data.payload.ip.src[1] = 168;
	buffer_u.cooked_data.payload.ip.src[2] = 0;
	buffer_u.cooked_data.payload.ip.src[3] = 1;

	buffer_u.cooked_data.payload.ip.dst[0] = binary_target_ip[0];
	buffer_u.cooked_data.payload.ip.dst[1] = binary_target_ip[1];
	buffer_u.cooked_data.payload.ip.dst[2] = binary_target_ip[2];
	buffer_u.cooked_data.payload.ip.dst[3] = binary_target_ip[3];
	buffer_u.cooked_data.payload.ip.sum = htons((~ipchksum((uint8_t *)&buffer_u.cooked_data.payload.ip) & 0xffff));

	buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(8080);
	buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(8080);
	buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

	/* Fill UDP payload */
	memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, message_buffer_size);

	memcpy(socket_address.sll_addr, dst_mac, 6);
	if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
		printf("Send failed\n");
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char interface_name[IFNAMSIZ];
	if (argc > 1)
		strcpy(interface_name, argv[1]);
	else
		strcpy(interface_name, DEFAULT_IF);

	_irc_send_udp_packet(interface_name, 8080, 8080, src_mac, dst_mac, "ABCDEFGHIJKLMNOPQRSTUVXAA");
	return 0;
	struct ifreq if_idx, if_mac, ifopts, if_ip;
	struct sockaddr_ll socket_address;
	int sockfd;
	uint8_t msg[] = "ABCDEFGHIJKLMNOPQRSTUVXAA";
    int message_buffer_size = strlen(msg)+1;



	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd == -1) {
		perror("socket error\n");
	}

	int op_result;

	printf("Setting interface: ");
	strncpy(ifopts.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	printf("%d\n", op_result);

	printf("Setting promiscuous mode: ");
	ifopts.ifr_flags |= IFF_PROMISC;
	op_result = ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	printf("%d\n", op_result);


	printf("Getting the index of the interface: ");
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFINDEX, &if_idx);
	printf("%d\n", op_result);
	if (op_result < 0) {
		printf("could not get index from interface name (SIOCGIFINDEX)\n");
	}
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;

	printf("Getting the MAC of the interface: ");
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFHWADDR, &if_mac);
	printf("%d\n", op_result);
	if (op_result < 0) {
		return printf("could not get mac from interface (SIOCGIFHWADDR)\n");
	}
	memcpy(binary_origin_mac, if_mac.ifr_hwaddr.sa_data, 6);

	printf("Getting the IP of the interface: ");
	memset(&if_ip, 0, sizeof(struct ifreq));
	strncpy(if_ip.ifr_name, interface_name, IFNAMSIZ-1);
 	op_result = ioctl(sockfd, SIOCGIFADDR, &if_ip);
 	printf("%d\n", op_result);

 	char ipv4_string[64];
 	snprintf(ipv4_string, 64, "%s", inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));

	printf("Source IP: %s\n", ipv4_string);

	/* Fill the Ethernet frame header */
	memcpy(buffer_u.cooked_data.ethernet.dst_addr, bcast_mac, 6);
	memcpy(buffer_u.cooked_data.ethernet.src_addr, src_mac, 6);
	buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

	buffer_u.cooked_data.payload.ip.ver = 0x45;
	buffer_u.cooked_data.payload.ip.tos = 0x00;
	buffer_u.cooked_data.payload.ip.len = htons(sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.ip.id = htons(0x00);
	buffer_u.cooked_data.payload.ip.off = htons(0x00);
	buffer_u.cooked_data.payload.ip.ttl = 50;
	buffer_u.cooked_data.payload.ip.proto = 17; //0xff;
	buffer_u.cooked_data.payload.ip.sum = htons(0x0000);

	/* Fill IP header data. Fill all fields and a zeroed CRC field, then update the CRC */
	buffer_u.cooked_data.payload.ip.src[0] = 127;
	buffer_u.cooked_data.payload.ip.src[1] = 0;
	buffer_u.cooked_data.payload.ip.src[2] = 0;
	buffer_u.cooked_data.payload.ip.src[3] = 1;
	buffer_u.cooked_data.payload.ip.dst[0] = 255;
	buffer_u.cooked_data.payload.ip.dst[1] = 255;
	buffer_u.cooked_data.payload.ip.dst[2] = 255;
	buffer_u.cooked_data.payload.ip.dst[3] = 255;
	buffer_u.cooked_data.payload.ip.sum = htons((~ipchksum((uint8_t *)&buffer_u.cooked_data.payload.ip) & 0xffff));
	printf("checksum is %8x\n", buffer_u.cooked_data.payload.ip.sum);
	buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(8080);
	buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(8080);
	buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

	memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, message_buffer_size);

	memcpy(socket_address.sll_addr, dst_mac, 6);
	if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0)
		printf("Send failed\n");

	return 0;
}