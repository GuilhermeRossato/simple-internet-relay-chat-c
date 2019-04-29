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
 * Listen for raw ethernet packets through an interface
 * The MAC address passed is used to filter ethernet packets (also accepts broadcast)
 *
 * @param interface  The interface to be used for sending the message
 * @param this_mac	 The MAC address describing when to actually listen to a ethernet packet
 * @param callback   The function to be called when a message is received
 *                   Callback must receive at least 2 parameters and at most 3 parameters:
 *                   1. the message char array
 *                   2. the size of the received message
 *                   3. a pointer to a [message_data_type] struct containing information about the sender
 *
 * @return           Success Indicator (1 when succeded, 0 when it fails)
 */
int irc_server(char * interface, char * this_mac, int (*callback)(char *, int, void*));

/**
 * Stops a server if it is in operation, breaking its inner loop
 *
 * @return Success indicator (1 when succeded, 0 when it fails)
 */
int irc_stop_server();

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


	char * binary_origin_mac = msg_data->origin_mac;
	char * binary_target_mac = msg_data->target_mac;

/*
	printf("Received from %02X:%02X:%02X:%02X\n", binary_origin_mac[0] & 0xFF, binary_origin_mac[1] & 0xFF, binary_origin_mac[2] & 0xFF, binary_origin_mac[3] & 0xFF);
	printf("Received to   %02X:%02X:%02X:%02X\n", binary_target_mac[0] & 0xFF, binary_target_mac[1] & 0xFF, binary_target_mac[2] & 0xFF, binary_target_mac[3] & 0xFF);
	printf("Received %d bytes: \"%s\"\n", buffer_size, buffer);
*/

	int (*callback)(char *, int, void *);
	callback = msg_data->func;
	callback(buffer+header_size, buffer_size-header_size, (void *) msg_data);
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

	unsigned long * check = (unsigned long *) buffer;
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

int irc_is_server_running;

int irc_stop_server() {
	if (irc_is_server_running == 1) {
		irc_is_server_running = 0;
		return 1;
	}
	return 0;
}

int irc_server(char * interface, char * this_mac, int (*callback)(char *, int, void*)) {
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

	unsigned char buffer[BUFFSIZE+2];
	int length;
	message_data_type msg_data;
	msg_data.func = callback;
	unsigned char target_mac[6];
	unsigned char origin_mac[6];
	// Convert input to binary mac if it is necessary
	if (_irc_is_string_mac(this_mac)) {
		_irc_mac_to_binary(this_mac, buffer);
		memcpy((void *) this_mac, buffer, 6);
	}
	irc_is_server_running = 1;
	int is_broadcast, is_targeted, is_origin_self;

	printf(
		"Starting server at interface %s with MAC %02X:%02X:%02X:%02X:%02X:%02X\n",
		interface,
		this_mac[0] & 0xFF,
		this_mac[1] & 0xFF,
		this_mac[2] & 0xFF,
		this_mac[3] & 0xFF,
		this_mac[4] & 0xFF,
		this_mac[5] & 0xFF
	);

	while (1) {
		if (irc_is_server_running != 1) {
			break;
		}
		length = recvfrom(sockd,(char *) &buffer, BUFFSIZE+1, 0x0, NULL, NULL);
		if (
			length >= 5 &&
			(buffer[0] & 0xFF) == 0 &&
			(buffer[1] & 0xFF) == 0 &&
			(buffer[2] & 0xFF) == 0 &&
			(buffer[3] & 0xFF) == 0 &&
			(buffer[4] & 0xFF) == 0 &&
			(buffer[5] & 0xFF) == 0
		) {
			continue;
		}
		/* debug_print_server("Received %d bytes", length); */
		if (length <= 20) {
			continue;
		} else {
			memcpy((void *) target_mac, buffer, 6);
			memcpy((void *) origin_mac, &buffer[6], 6);
			is_broadcast = (
				target_mac[0] == 0xFF &&
				target_mac[1] == 0xFF &&
				target_mac[2] == 0xFF &&
				target_mac[3] == 0xFF &&
				target_mac[4] == 0xFF &&
				target_mac[5] == 0xFF
			);
			is_targeted = (
				(target_mac[0] & 0xFF) == (this_mac[0] & 0xFF) &&
				(target_mac[1] & 0xFF) == (this_mac[1] & 0xFF) &&
				(target_mac[2] & 0xFF) == (this_mac[2] & 0xFF) &&
				(target_mac[3] & 0xFF) == (this_mac[3] & 0xFF) &&
				(target_mac[4] & 0xFF) == (this_mac[4] & 0xFF) &&
				(target_mac[5] & 0xFF) == (this_mac[5] & 0xFF)
			);
			is_origin_self = (
				(origin_mac[0] & 0xFF) == (this_mac[0] & 0xFF) &&
				(origin_mac[1] & 0xFF) == (this_mac[1] & 0xFF) &&
				(origin_mac[2] & 0xFF) == (this_mac[2] & 0xFF) &&
				(origin_mac[3] & 0xFF) == (this_mac[3] & 0xFF) &&
				(origin_mac[4] & 0xFF) == (this_mac[4] & 0xFF) &&
				(origin_mac[5] & 0xFF) == (this_mac[5] & 0xFF)
			);
			if (is_origin_self) {
				if (DEBUG_SERVER) {
					printf("Skipping ethernet packet sent by myself\n");
				}
				continue;
			}
			if (!is_broadcast && !is_targeted) {
				if (DEBUG_SERVER) {
					printf(
						"[Ethernet] Ignored packet addressed to %02X:%02X:%02X:%02X:%02X:%02X\n",
						target_mac[0] & 0xFF,
						target_mac[1] & 0xFF,
						target_mac[2] & 0xFF,
						target_mac[3] & 0xFF,
						target_mac[4] & 0xFF,
						target_mac[5] & 0xFF
					);
				}
				continue;
			}
			irc_on_ethernet_packet_received(&msg_data, buffer, length+1);
		}
	}
}
