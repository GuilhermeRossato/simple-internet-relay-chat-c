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

// Send Interface

/**
 * Sends a IRC message through an interface
 *
 * @return           Success Indicator (1 when succeded, 0 when it fails)
 */
int irc_send(char * message, char * interface, char * origin_mac, char * origin_ip, char * target_mac, char * target_ip);

// Send Implementation

#define DEBUG_SEND 0
#define debug_print_send(fmt, ...) \
        do { if (DEBUG_SEND) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)

char bcast_mac[6] =	{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

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

int _irc_send_udp_packet(
	char * interface_name,

	uint8_t * binary_origin_mac,
	uint8_t * binary_origin_ip,
	int origin_port,

	uint8_t * binary_target_mac,
	uint8_t * binary_target_ip,
	int target_port,

	unsigned char * message,
	int message_size
);

int irc_send_udp_data(
	char * interface,

	uint8_t * origin_mac,
	uint8_t * origin_ip,
	int origin_port,

	uint8_t * target_mac,
	uint8_t * target_ip,
	int target_port,

	unsigned char * message,
	int message_size
) {
	uint8_t binary_origin_mac[6+1];
	uint8_t binary_origin_ip[4+1];

	uint8_t binary_target_mac[6+1];
	uint8_t binary_target_ip[4+1];

	_irc_ip_to_binary(origin_ip, (uint8_t *) binary_origin_ip);
	_irc_mac_to_binary(origin_mac, (uint8_t *) binary_origin_mac);

	_irc_ip_to_binary(target_ip, (uint8_t *) binary_target_ip);
	_irc_mac_to_binary(target_mac, (uint8_t *) binary_target_mac);

/*
	printf("Sending from %02X:%02X:%02X:%02X\n", binary_origin_mac[0] & 0xFF, binary_origin_mac[1] & 0xFF, binary_origin_mac[2] & 0xFF, binary_origin_mac[3] & 0xFF);
	printf("Sending to   %02X:%02X:%02X:%02X\n", binary_target_mac[0] & 0xFF, binary_target_mac[1] & 0xFF, binary_target_mac[2] & 0xFF, binary_target_mac[3] & 0xFF);
	printf("Sending %d bytes: \"%s\"\n", message_size, message);
*/

	int result;

	result = _irc_send_udp_packet(
		interface,

		binary_origin_mac,
		binary_origin_ip,
		origin_port,

		binary_target_mac,
		binary_target_ip,
		target_port,

		message,
		message_size
	);

	return result;
}

int _irc_send_udp_packet(
	char * interface_name,

	uint8_t * binary_origin_mac,
	uint8_t * binary_origin_ip,
	int origin_port,

	uint8_t * binary_target_mac,
	uint8_t * binary_target_ip,
	int target_port,

	unsigned char * message,
	int message_size
) {
	struct ifreq if_idx, ifopts, if_ip;
	struct sockaddr_ll socket_address;
	int sockfd;
	uint8_t * msg = message;
    int message_buffer_size = message_size;

	sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if (sockfd == -1) {
		return irc_error_could_not("open socket");
	}

	int op_result;

	strncpy(ifopts.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
	debug_print_send("Setting interface: %d\n", op_result);

	ifopts.ifr_flags |= IFF_PROMISC;
	op_result = ioctl(sockfd, SIOCSIFFLAGS, &ifopts);
	debug_print_send("Setting promiscuous mode: %d\n", op_result);
	if (op_result < 0) {
		shutdown(sockfd, 0);
		return irc_error_could_not("set the interface to promiscuous mode");
	}

	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, interface_name, IFNAMSIZ-1);
	op_result = ioctl(sockfd, SIOCGIFINDEX, &if_idx);
	debug_print_send("Getting the index of the interface: %d\n", op_result);
	if (op_result < 0) {
		shutdown(sockfd, 0);
		return irc_error_could_not("get the index of the interface");
	}
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	socket_address.sll_halen = ETH_ALEN;

	memcpy(buffer_u.cooked_data.ethernet.dst_addr, binary_target_mac, 6);
	memcpy(buffer_u.cooked_data.ethernet.src_addr, binary_origin_mac, 6);
	buffer_u.cooked_data.ethernet.eth_type = htons(ETH_P_IP);

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

	buffer_u.cooked_data.payload.udp.udphdr.src_port = htons(origin_port);
	buffer_u.cooked_data.payload.udp.udphdr.dst_port = htons(target_port);
	buffer_u.cooked_data.payload.udp.udphdr.udp_len = htons(sizeof(struct udp_hdr) + message_buffer_size);
	buffer_u.cooked_data.payload.udp.udphdr.udp_chksum = 0;

	/* Fill UDP payload */
	memcpy(buffer_u.raw_data + sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr), msg, message_buffer_size);

	debug_print_send(
		"Sending %d bytes to mac: %02X:%02X:%02X:%02X:%02X:%02X\n",
		message_size,
		binary_target_mac[0],
		binary_target_mac[1],
		binary_target_mac[2],
		binary_target_mac[3],
		binary_target_mac[4],
		binary_target_mac[5]
	);
	memcpy(socket_address.sll_addr, binary_target_mac, 6);
	if (sendto(sockfd, buffer_u.raw_data, sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr) + message_buffer_size, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
		shutdown(sockfd, 0);
		return irc_error_could_not("send the message");
	}

	shutdown(sockfd, 0);
	return 1;
}

int irc_send(char * message, char * interface, char * origin_mac, char * origin_ip, char * target_mac, char * target_ip) {
	int length = strlen(message) + 1;
	int buffer_size = length + 4 + 1;
	char * buffer = (char *) malloc((sizeof(char) * buffer_size) + 1);
	snprintf(buffer, buffer_size-1, "IRC%s", message);
	buffer[buffer_size-1] = '\0';

	int result = irc_send_udp_data(
		interface,
		origin_mac,
		origin_ip,
		8080,
		target_mac,
		target_ip,
		8081,
		buffer,
		buffer_size
	);

	free(buffer);

	return result;
}
