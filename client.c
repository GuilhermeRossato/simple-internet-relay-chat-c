#include "./src/irc.h"
#include <stdlib.h>
#include <stdio.h>

int usage(char * origin) {
	printf("\n");
	printf("Usage:\n");
	printf("\n");
	printf(" %s <interface>\n", origin);
	printf("\n");
	printf("The first parameter must be an interface, here's a list of available interfaces:\n");

	char interface_name[64];

	int count = 0;

	while (irc_put_ethernet_interface_name_by_id(count, interface_name, 64)) {
		printf("[%d] %s\n", count, interface_name);
		count++;
	}

	printf("\n");
}

int main(int argn, char ** argc) {
	if (argn > 0 && argn != 2) {
		return usage(argc[0]);
	}

	char * interface_name = argc[1];

	unsigned char origin_mac[16];
	if (!irc_put_ethernet_interface_address_by_name(interface_name, origin_mac, 16)) {
		printf("Interface not found: %s\n", interface_name);
		exit(1);
	}

	printf("Interface: %s [%02x:%02x:%02x:%02x:%02x:%02x]\n", interface_name, origin_mac[0], origin_mac[1], origin_mac[2], origin_mac[3], origin_mac[4], origin_mac[5]);

	irc_send_udp_data(
		interface_name,
		8080,
		8080,
		origin_mac,
		"255.255.255.255",
		"Hello World"
	);

	return 0;
}