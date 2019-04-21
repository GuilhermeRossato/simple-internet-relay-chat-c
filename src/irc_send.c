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
#include <netdb.h>
#include "../raw/raw.h"

#ifndef NI_MAXHOST
	#define NI_MAXHOST	64
#endif
#ifndef NI_NUMERICHOST
	#define NI_NUMERICHOST	64
#endif

char this_mac[6];
char bcast_mac[6] =	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
char dst_mac[6] =	{0x00, 0x00, 0x00, 0x22, 0x22, 0x22};
char src_mac[6] =	{0x00, 0x00, 0x00, 0x33, 0x33, 0x33};

union eth_buffer buffer_u;

uint32_t irc_ipchksum(uint8_t *packet)
{
	uint32_t sum=0;
	uint16_t i;

	for(i = 0; i < 20; i += 2)
		sum += ((uint32_t)packet[i] << 8) | (uint32_t)packet[i + 1];
	while (sum & 0xffff0000)
		sum = (sum & 0xffff) + (sum >> 16);
	return sum;
}

int _irc_is_string_mac(char * mac) {
	int i, is_separator, is_number, is_letter;
	for (i=0;i<6;i++) {
		is_separator = (mac[i] == ':');
		is_number = (mac[i] >= '0' && mac[i] <= '9');
		is_letter = ((mac[i] >= 'a' && mac[i] <= 'f') || (mac[i] >= 'A' && mac[i] <= 'F'));

		if ((i == 2 || i == 5) && !is_separator) {
			return 0;
		}

		if (!is_number && !is_letter) {
			return 0;
		}
	}
	return 1;
}


int _irc_is_string_ip(char * ip) {
	int i, is_separator, is_number;
	for (i=0;i<4;i++) {
		is_separator = (ip[i] == '.');
		is_number = (ip[i] >= '0' && ip[i] <= '9');
		if (i == 0 && !is_number) {
			return 0;
		}
		if (!is_number && !is_separator) {
			return 0;
		}
	}
	return 1;
}

int _irc_mac_to_binary(char * mac, unsigned int * binary_mac) {
	if (!_irc_is_string_mac(mac)) {
		memcpy(binary_mac, mac, 6);
		return 1;
	}
	sscanf(mac,"%x:%x:%x:%x:%x:%x",&binary_mac[0],&binary_mac[1],&binary_mac[2],&binary_mac[3],&binary_mac[4],&binary_mac[5]);
	return 1;
}

int _irc_ip_to_binary(char * ip, unsigned char * binary_ip) {
	if (!_irc_is_string_ip(ip)) {
		memcpy(binary_ip, ip, 6);
		return 1;
	}
	sscanf(ip,"%d.%d.%d.%d",(int *) &binary_ip[0],(int *) &binary_ip[1],(int *) &binary_ip[2],(int *) &binary_ip[3]);
	return 1;
}

int _irc_send_udp_packet(
	char * interface_name,
	int origin_port,
	int target_port,
	uint8_t * binary_target_mac,
	uint8_t * binary_target_ip,
	char * message
);

int irc_send_udp_data(
	char * interface,
	int origin_port,
	int target_port,
	char * target_mac,
	char * target_ip,
	char * message
) {
	uint8_t binary_mac[6+1];
	uint8_t binary_ip[4+1];

	_irc_mac_to_binary(target_mac, (unsigned int *) binary_mac);
	_irc_ip_to_binary(target_ip, (char *) binary_ip);

	printf("Sending to:\n");
	printf("Mac : %02x:%02x:%02x:%02x...\n", binary_mac[0], binary_mac[1], binary_mac[2], binary_mac[3]);
	printf("Ip  : %d.%d.%d.%d\n", binary_ip[0], binary_ip[1], binary_ip[2], binary_ip[3]);
	printf("Port: %d\n", target_port);

	return _irc_send_udp_packet(interface, origin_port, target_port, binary_mac, binary_ip, message);
}

int _irc_send_udp_packet(
	char * interface_name,
	int origin_port,
	int target_port,
	uint8_t * binary_target_mac,
	uint8_t * binary_target_ip,
	char * message
) {
	struct ifreq if_idx, if_mac, if_ip, ifopts;
	struct sockaddr_ll socket_address;
	int sockfd, numbytes;
	uint8_t * msg = message;
    int message_buffer_size = strlen(msg)+1;
    int op_result;
    uint8_t binary_origin_mac[6+1];
    uint8_t binary_origin_ip[4+1];
	char origin_name[NI_MAXHOST];

	/* Open RAW socket */
	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd == -1) {
		return irc_error_could_not("create socket");
	}

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
		return irc_error_could_not("get index from interface name");
	}
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;

	/* Get the MAC address of the interface */
	printf("Getting the MAC of the interface: ");
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFHWADDR, &if_mac);
	printf("%d\n", op_result);
	if (op_result < 0) {
		return irc_error_could_not("get mac from interface (SIOCGIFHWADDR)");
	}
	memcpy(binary_origin_mac, if_mac.ifr_hwaddr.sa_data, 6);

	printf("Getting the IP of the interface: ");
	memset(&if_ip, 0, sizeof(struct ifreq));
	strncpy(if_ip.ifr_name, interface_name, IFNAMSIZ-1);
 	op_result = ioctl(sockfd, SIOCGIFADDR, &if_ip);
 	printf("%d\n", op_result);

 	char ipv4_string[64];
 	snprintf(ipv4_string, 64, "%s", inet_ntoa(((struct sockaddr_in *)&if_ip.ifr_addr)->sin_addr));

	_irc_ip_to_binary(ipv4_string, (char *) binary_origin_ip);
	printf("src IP: %d.%d.%d.%d\n", binary_origin_ip[0], binary_origin_ip[1], binary_origin_ip[2], binary_origin_ip[3]);
	printf("dst IP: %d.%d.%d.%d\n", binary_target_ip[0], binary_target_ip[1], binary_target_ip[2], binary_target_ip[3]);


	/* Fill the Ethernet frame header */
	memcpy(buffer_u.cooked_data.ethernet.dst_addr, binary_target_mac, 6);
	memcpy(buffer_u.cooked_data.ethernet.src_addr, binary_origin_mac, 6);
	buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

	/* Fill IP header data. Fill all fields and a zeroed CRC field, then update the CRC */
	buffer_u.cooked_data.payload.ip.ver = 0x45;
	buffer_u.cooked_data.payload.ip.tos = 0x00;
	buffer_u.cooked_data.payload.ip.len = htons(sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.ip.id = htons(0x00);
	buffer_u.cooked_data.payload.ip.off = htons(0x00);
	buffer_u.cooked_data.payload.ip.ttl = 50;
	buffer_u.cooked_data.payload.ip.proto = 17; //0xff;
	buffer_u.cooked_data.payload.ip.sum = htons(0x0000);

	buffer_u.cooked_data.payload.ip.src[0] = binary_origin_ip[0];
	buffer_u.cooked_data.payload.ip.src[1] = binary_origin_ip[1];
	buffer_u.cooked_data.payload.ip.src[2] = binary_origin_ip[2];
	buffer_u.cooked_data.payload.ip.src[3] = binary_origin_ip[3];

	buffer_u.cooked_data.payload.ip.dst[0] = binary_target_ip[0];
	buffer_u.cooked_data.payload.ip.dst[1] = binary_target_ip[1];
	buffer_u.cooked_data.payload.ip.dst[2] = binary_target_ip[2];
	buffer_u.cooked_data.payload.ip.dst[3] = binary_target_ip[3];
	buffer_u.cooked_data.payload.ip.sum = htons((~irc_ipchksum((uint8_t *)&buffer_u.cooked_data.payload.ip) & 0xffff));

	/* Fill UDP header */
	buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(origin_port);
	buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(target_port);
	buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

	/* Fill UDP payload */
	memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, message_buffer_size);

	/* Send it.. */
	memcpy(socket_address.sll_addr, dst_mac, 6);
	if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
		printf("Send failed\n");
	}

	return 0;
}
