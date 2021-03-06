#include "irc_shared.h"
#include <malloc.h>

// User Interface

int irc_create_user(char * origin, char * user_name);
int irc_destroy_user_by_name(char * user_name);
int irc_destroy_user_by_origin(char * origin);
int irc_check_user_by_origin(char * origin);
char * irc_get_user_origin_by_id(int id);
char * irc_get_user_name_by_origin(char * origin);

// User Implementation

int irc_check_user_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(irc_user, origin);
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
	irc_node * node = irc_search_node_by_name(irc_user, user_name);
	if (!node) {
		return irc_error_object_not_found("user with name");
	}
	irc_destroy_node(node);
	return 1;
}

int irc_destroy_user_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(irc_user, origin);
	if (!node) {
		return irc_error_object_not_found("user with origin");
	}
	irc_destroy_node(node);
	return 1;
}

char * irc_get_user_origin_by_id(int id) {
	int this_id = 0;
	irc_node * node = irc_root_node;
	while (node) {
		if (node->type == irc_user) {
			if (this_id == id) {
				return node->origin;
			}
			this_id++;
		}
		node = node->next;
	}
	return 0;
}

char * irc_get_user_name_by_origin(char * origin) {
	irc_node * node = irc_search_node_by_origin(irc_user, origin);
	if (!node) {
		return 0;
	}
	return node->name;
}

