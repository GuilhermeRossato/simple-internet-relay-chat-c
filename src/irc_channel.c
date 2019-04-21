#include "irc_shared.h"
#include <malloc.h>

// Channel Interface

int irc_create_channel(char * origin, char * channel_name);
int irc_destroy_channel_by_name(char * channel_name);
int irc_destroy_channel_by_origin(char * origin);
int irc_check_channel_exists(char * channel_name);
int irc_check_channel_by_origin(char * origin);

// Channel Implementation

int irc_check_channel_exists(char * channel_name) {
	irc_node * node = irc_search_node_by_name(irc_channel, channel_name);
	if (node != 0) {
		return 1;
	}
	return 0;
}


int irc_check_channel_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(irc_channel, origin);
	if (node != 0) {
		return 1;
	}
	return 0;
}

int irc_create_channel(char * origin, char * channel_name) {
	if (irc_check_channel_exists(channel_name)) {
		return irc_error_object_already_exists("channel with name");
	}

	irc_node * node = irc_create_node(irc_channel, 0, origin, channel_name);

	if (!node) {
		return irc_error_invalid_something("created channel");
	}
	return 1;
}

int irc_destroy_channel_by_name(char * channel_name) {
	irc_node * node = irc_search_node_by_name(irc_channel, channel_name);
	if (!node) {
		return irc_error_object_not_found("channel with name");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_channel_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(irc_channel, origin);
	if (!node) {
		return irc_error_object_not_found("channel with origin");
	}
	irc_destroy_node(node);
	return 1;
}