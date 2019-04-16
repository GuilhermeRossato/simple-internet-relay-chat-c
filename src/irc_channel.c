#include "irc_shared.h"
#include <malloc.h>

// Channel Interface

int irc_create_channel(char * mac, char * channel_name);
int irc_destroy_channel_by_name(char * channel_name);
int irc_destroy_channel_by_mac(char * mac);
int irc_check_channel_exists(char * channel_name);
int irc_check_channel_by_mac(char * mac);

// Channel Implementation

int irc_check_channel_exists(char * channel_name) {
	irc_node * node = irc_search_node_by_name(channel_name);
	if (node != 0) {
		return 1;
	}
	return 0;
}


int irc_check_channel_by_mac(char * mac) {
	irc_node * node = irc_search_node_by_mac(mac);
	if (node != 0) {
		return 1;
	}
	return 0;
}

int irc_create_channel(char * mac, char * channel_name) {
	if (irc_check_channel_exists(channel_name)) {
		return irc_error_object_already_exists("channel with name");
	}

	irc_node * node = irc_create_node(irc_channel, 0, mac, channel_name);

	if (!node) {
		return irc_error_invalid_something("created channel");
	}
	return 1;
}

int irc_destroy_channel_by_name(char * channel_name) {
	irc_node * node = irc_search_node_by_name(channel_name);
	if (!node) {
		return irc_error_object_not_found("channel with name");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_channel_by_mac(char * mac) {
	irc_node * node = irc_search_node_by_mac(mac);
	if (!node) {
		return irc_error_object_not_found("channel with mac");
	}
	irc_destroy_node(node);
	return 1;
}