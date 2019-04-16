#include "irc_shared.h"
#include <malloc.h>

// User Interface

int irc_create_user(char * origin, char * user_name);
int irc_destroy_user_by_name(char * user_name);
int irc_destroy_user_by_origin(char * origin);
int irc_check_user_by_origin(char * origin);

// User Implementation

int irc_check_user_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(origin);
	if (node != 0) {
		return 1;
	}
	return 0;
}

int irc_unique_user_id = 0;

int irc_create_user(char * origin, char * user_name) {
	if (irc_check_user_by_origin(origin)) {
		return irc_error_object_already_exists("user with origin");
	}

	irc_unique_user_id++;

	irc_node * node = irc_create_node(irc_user, irc_unique_user_id, origin, user_name);

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

int irc_destroy_user_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(origin);
	if (!node) {
		return irc_error_object_not_found("user with origin");
	}
	irc_destroy_node(node);
	return 1;
}