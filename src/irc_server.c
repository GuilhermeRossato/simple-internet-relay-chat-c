#include "irc_shared.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>  //estrutura ifr
#include <netinet/ether.h> //header ethernet
#include <netinet/in.h> //definicao de protocolos
#include <arpa/inet.h> //funcoes para manipulacao de enderecos IP

#include <netinet/in_systm.h> //tipos de dados

#include <unistd.h> // exec
#include <ifaddrs.h> // Interfaces disponiveis

// Server Interface

/**
 * Sends a IRC broadcast message through an interface
 *
 * @param interface  The interface to be used for sending the message
 * @param callback   The function to be called when a message is received
 *
 * @return           Success Indicator (1 when succeded, 0 when it fails)
 */
int irc_server(char * interface, char * this_mac, int (*callback)(char *, int));

// Server Implementation

#define BUFFSIZE 1518

#define DEBUG_SERVER 0
#define debug_print_server(fmt, ...) \
        do { if (DEBUG_SERVER) fprintf(stderr, "%s:%d:%s():\n" fmt "\n", __FILE__, __LINE__, __func__, __VA_ARGS__); } while (0)

typedef enum {
	eth_UNKNOWN,
	eth_RAW_ETHERNET,
	eth_IPv4 = 0x0800,
	eth_IPv6 = 0x86dd,
	eth_ARP = 0x0806,
	eth_IPX = 0x8137
} ethernet_content_type;

typedef struct message_data_type {
	char target_mac[6];
	char origin_mac[6];
	ethernet_content_type type;
	char target_ip[6];
	char origin_ip[6];

	int source_port;
	int target_port;

	void * func;
} message_data_type;

int irc_on_content_received(message_data_type * msg_data, unsigned char * buffer, unsigned int buffer_size) {
	if (buffer[0] != 'I' || buffer[1] != 'R' || buffer[2] != 'C') {
		// Signature failed: not this protocol
		return 0;
	}
	int header_size = 3;
	int actual_size = buffer_size-header_size;

	debug_print_server(
		"        [IRC] Length: %d",
		actual_size
	);

	int (*callback)(char *, int);
	callback = msg_data->func;
	callback(buffer+header_size, buffer_size-header_size);
}

int irc_on_udp_packet_received(message_data_type * msg_data, unsigned char * buffer, unsigned int buffer_size) {
	msg_data->source_port = ((int) ((int) buffer[0] & 0xff) << 8) + ((int) buffer[1] & 0xff);
	msg_data->target_port = ((int) ((int) buffer[2] & 0xff) << 8) + ((int) buffer[3] & 0xff);
	int total_length = ((int) (buffer[4] & 0xff) << 8) + buffer[5] & 0xff;
	int actual_size = ((buffer_size < total_length)?buffer_size:total_length);

	debug_print_server(
		"      [UDP] Size: %d - Source Port: %5d - Target Port %5d",
		actual_size,
		msg_data->source_port,
		msg_data->target_port
	);
	irc_on_content_received(msg_data, buffer + 8, actual_size - 8);
	//on_bootstrap_received(buffer + 8, buffer_size - 8);
}

int irc_on_ip_packet_received(message_data_type * msg_data, unsigned char * buffer, unsigned int buffer_size) {
	int ip_version = buffer[0] & 0xf0 >> 2;
	int header_length = (buffer[0] & 0x0f)*4;
	int total_length = ( (int) (buffer[2] & 0xff) << 8) + (int) (buffer[3] & 0xff);

	int actual_size = ((buffer_size < total_length)?buffer_size:total_length);

	if (header_length == 20 && ip_version == 4) {
		unsigned char protocol = buffer[9] & 0xff;

		memcpy((void *) msg_data->origin_ip, &buffer[12], 6);
		memcpy((void *) msg_data->target_ip, &buffer[16], 6);

		debug_print_server(
			"    [IP] Size: %d - Header %d - Protocol %d - Origin %d.%d.%d.%d",
			actual_size,
			header_length,
			protocol,
			msg_data->target_ip[0] & 0xFF,
			msg_data->target_ip[1] & 0xFF,
			msg_data->target_ip[2] & 0xFF,
			msg_data->target_ip[3] & 0xFF
		);

		if (protocol == 17) {
			irc_on_udp_packet_received(
				msg_data,
				buffer + header_length,
				actual_size - header_length
			);
		}
	}
}

int irc_on_ethernet_packet_received(message_data_type * msg_data, unsigned char * buffer, unsigned int buffer_size) {
	int type;

	unsigned long * check = (unsigned long * ) buffer;
	if (*check == 0) {
		return 0;
	}

	memcpy((void *) msg_data->target_mac, buffer, 6);
	memcpy((void *) msg_data->origin_mac, buffer+6, 6);
	int type_id = (buffer[12] << 8) + buffer[13];

	if (type_id <= 1500) {
		msg_data->type = eth_RAW_ETHERNET;
	} else if (type_id == 0x0800) {
		msg_data->type = eth_IPv4;
	} else if (type_id == 0x0806) {
		msg_data->type = eth_ARP;
	} else if (type_id == 0x8137) {
		msg_data->type = eth_IPX;
	} else if (type_id == 0x86dd) {
		msg_data->type = eth_IPv6;
	} else {
		msg_data->type = eth_UNKNOWN;
	}

	if (msg_data->type == eth_IPv4) {
		debug_print_server(
			"# [Ethernet] Size: %d - Origin: %02X:%02X:%02X:%02X:%02X:%02X - Target: %02X:%02X:%02X:%02X:%02X:%02X",
			buffer_size,
			msg_data->origin_mac[0] & 0xFF,
			msg_data->origin_mac[1] & 0xFF,
			msg_data->origin_mac[2] & 0xFF,
			msg_data->origin_mac[3] & 0xFF,
			msg_data->origin_mac[4] & 0xFF,
			msg_data->origin_mac[5] & 0xFF,
			msg_data->target_mac[0] & 0xFF,
			msg_data->target_mac[1] & 0xFF,
			msg_data->target_mac[2] & 0xFF,
			msg_data->target_mac[3] & 0xFF,
			msg_data->target_mac[4] & 0xFF,
			msg_data->target_mac[5] & 0xFF
		);
		irc_on_ip_packet_received(msg_data, buffer+14, buffer_size-14);
	}
}

int irc_server(char * interface, char * this_mac, int (*callback)(char *, int)) {
	int sockd;
	if ((sockd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) < 0) {
		return irc_error_could_not("open raw socket");
	}
	int i;
	struct ifreq ifr;

	strcpy(ifr.ifr_name, interface);
	if (ioctl(sockd, SIOCGIFINDEX, &ifr) < 0) {
		return irc_error_could_not("set the interface to promicious mode");
	}
	ioctl(sockd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_PROMISC;
	ioctl(sockd, SIOCSIFFLAGS, &ifr);

	unsigned char buffer[BUFFSIZE+1];
	int length;
	message_data_type msg_data;
	msg_data.func = callback;
	unsigned char target_mac[6];
	while (1) {
		length = recvfrom(sockd,(char *) &buffer, sizeof(buffer)+1, 0x0, NULL, NULL);
		if (length <= 20) {
			continue;
		} else {
			memcpy((void *) target_mac, buffer, 6);
			int is_broadcast = (
				target_mac[0] == 0xFF &&
				target_mac[1] == 0xFF &&
				target_mac[2] == 0xFF &&
				target_mac[3] == 0xFF &&
				target_mac[4] == 0xFF &&
				target_mac[5] == 0xFF
			);
			int is_targeted = (
				target_mac[0] == this_mac[0] &&
				target_mac[1] == this_mac[1] &&
				target_mac[2] == this_mac[2] &&
				target_mac[3] == this_mac[3] &&
				target_mac[4] == this_mac[4] &&
				target_mac[5] == this_mac[5]
			);
			if (!is_broadcast && !is_targeted) {
				debug_print_server(
					"[Ethernet] Ignored packet addressed to %02X:%02X:%02X:%02X:%02X:%02X\n",
					target_mac[0] & 0xFF,
					target_mac[1] & 0xFF,
					target_mac[2] & 0xFF,
					target_mac[3] & 0xFF,
					target_mac[4] & 0xFF,
					target_mac[5] & 0xFF
				);
				continue;
			}
			irc_on_ethernet_packet_received(&msg_data, buffer, length+1);
		}
	}
}
