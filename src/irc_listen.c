#include "irc_shared.h"
#include <malloc.h>

// Listen Interface

int irc_create_listen(char * origin, char * channel_name);
int irc_destroy_listen_by_name(char * channel_name);
int irc_destroy_listen_by_origin(char * origin);
int irc_destroy_listen_by_origin_and_name(char * origin);
int irc_check_listen_by_origin(char * origin);
int irc_check_listen_by_origin_and_name(char * origin, char * channel_name);

// Listen Implementation

int irc_check_listen_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(origin);
	if (node) {
		return 1;
	}
	return 0;
}

int irc_check_listen_by_origin_and_name(char * origin, char * channel_name) {
	irc_node * node = irc_search_node_by_origin_and_name(origin, channel_name);
	if (node) {
		return 1;
	}
	return 0;
}

int irc_create_listen(char * origin, char * channel_name) {
	if (irc_check_listen_by_origin_and_name(origin, channel_name)) {
		return irc_error_object_already_exists("listen with origin and channel");
	}

	irc_node * node = irc_create_node(irc_listen, 0, origin, channel_name);

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

int irc_destroy_listen_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(origin);
	if (!node) {
		return irc_error_object_not_found("listen with origin");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_listen_by_origin_and_name(char * origin, char * channel_name) {
	irc_node * node = irc_search_node_by_origin_and_name(origin, channel_name);
	if (!node) {
		return irc_error_object_not_found("listen with origin");
	}
	irc_destroy_node(node);
	return 1;
}