#include "irc_shared.h"
#include <malloc.h>

// User Interface

int irc_create_user(char * mac, char * user_name);
int irc_destroy_user_by_name(char * user_name);
int irc_destroy_user_by_mac(char * mac);
int irc_check_user_by_mac(char * mac);

// User Implementation

int irc_check_user_by_mac(char * mac) {
	irc_node * node = irc_search_node_by_mac(mac);
	if (node != 0) {
		return 1;
	}
	return 0;
}

int irc_unique_user_id = 0;

int irc_create_user(char * mac, char * user_name) {
	if (irc_check_user_by_mac(mac)) {
		return irc_error_object_already_exists("user with mac");
	}

	irc_unique_user_id++;

	irc_node * node = irc_create_node(irc_user, irc_unique_user_id, mac, user_name);

	if (!node) {
		return irc_error_invalid_something("created user");
	}

	return 1;
}

int irc_destroy_user_by_name(char * user_name) {
	irc_node * node = irc_search_node_by_name(user_name);
	if (!node) {
		return irc_error_object_not_found("user with name");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_user_by_mac(char * mac) {
	irc_node * node = irc_search_node_by_mac(mac);
	if (!node) {
		return irc_error_object_not_found("user with mac");
	}
	irc_destroy_node(node);
	return 1;
}