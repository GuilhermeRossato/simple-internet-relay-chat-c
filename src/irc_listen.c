#include "irc_shared.h"
#include <malloc.h>

// Listen Interface

int irc_create_listen(char * mac, char * channel_name);
int irc_destroy_listen_by_name(char * channel_name);
int irc_destroy_listen_by_mac(char * mac);
int irc_destroy_listen_by_mac_and_name(char * mac);
int irc_check_listen_by_mac(char * mac);
int irc_check_listen_by_mac_and_name(char * mac, char * channel_name);

// Listen Implementation

int irc_check_listen_by_mac(char * mac) {
	irc_node * node = irc_search_node_by_mac(mac);
	if (node) {
		return 1;
	}
	return 0;
}

int irc_check_listen_by_mac_and_name(char * mac, char * channel_name) {
	irc_node * node = irc_search_node_by_mac_and_name(mac, channel_name);
	if (node) {
		return 1;
	}
	return 0;
}

int irc_create_listen(char * mac, char * channel_name) {
	if (irc_check_listen_by_mac_and_name(mac, channel_name)) {
		return irc_error_object_already_exists("listen with mac and channel");
	}

	irc_node * node = irc_create_node(irc_listen, 0, mac, channel_name);

	if (!node) {
		return irc_error_invalid_something("created listen");
	}

	return 1;
}

int irc_destroy_listen_by_name(char * channel_name) {
	irc_node * node = irc_search_node_by_name(channel_name);
	if (!node) {
		return irc_error_object_not_found("listen with name");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_listen_by_mac(char * mac) {
	irc_node * node = irc_search_node_by_mac(mac);
	if (!node) {
		return irc_error_object_not_found("listen with mac");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_listen_by_mac_and_name(char * mac, char * channel_name) {
	irc_node * node = irc_search_node_by_mac_and_name(mac, channel_name);
	if (!node) {
		return irc_error_object_not_found("listen with mac");
	}
	irc_destroy_node(node);
	return 1;
}