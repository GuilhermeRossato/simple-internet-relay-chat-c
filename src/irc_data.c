#include "irc_shared.h"

// Data Node Interface

irc_node * irc_create_node(enum irc_node_type type, int owner, unsigned char * mac, char * name)
irc_node * irc_search_node_by_mac(enum irc_node_type type, char * mac);
irc_node * irc_search_node_by_name(enum irc_node_type type, char * name);
irc_node * irc_search_node_by_mac_and_name(enum irc_node_type type, char * mac, char * name);
irc_node * irc_search_node_by_owner(enum irc_node_type type, int owner);
irc_node * irc_destroy_node(irc_node * node);

// Data Node Implementation

irc_node * _irc_search_node_by_next(irc_node * last) {
	irc_node * node = &irc_root_node;
	while (node && node->next && node->next != last) {
		node = node->next;
	}
	return node;
}

irc_node * irc_search_node_by_mac(enum irc_node_type type, char * mac) {
	irc_node * node = &irc_root_node;
	while (node) {
		if (node->type == type && _irc_compare_two_strings(node->mac, mac, 6)) {
			return node;
		}
		node = node->next;
	}
	return 0;
}

irc_node * irc_search_node_by_name(enum irc_node_type type, char * name) {
	irc_node * node = &irc_root_node;
	while (node) {
		if (node->type == type && _irc_compare_two_strings(node->name, name, IRC_BUFFER_SIZE)) {
			return node;
		}
		node = node->next;
	}
	return 0;
}

irc_node * irc_search_node_by_mac_and_name(enum irc_node_type type, char * mac, char * name) {
	irc_node * node = &irc_root_node;
	while (node) {
		if (
			node->type == type &&
			_irc_compare_two_strings(node->name, name, IRC_BUFFER_SIZE) &&
			_irc_compare_two_strings(node->mac, mac, 6)
		) {
			return node;
		}
		node = node->next;
	}
	return 0;
}

irc_node * irc_search_node_by_owner(enum irc_node_type type, int owner) {
	irc_node * node = &irc_root_node;
	while (node) {
		if (node->type == type && node->owner == owner) {
			return node;
		}
		node = node->next;
	}
	return 0;
}

irc_node * irc_create_node(enum irc_node_type type, int owner, unsigned char * mac, char * name) {
	if (type == irc_none) {
		return irc_error_invalid_something("type");
	}
	irc_node * node = (irc_node *) malloc(sizeof(irc_node));
	if (node == 0) {
		return irc_error_malloc_failed("new node");
	}
	if (irc_root_node == 0) {
		irc_root_node = node;
	} else {
		irc_last_node->next = node;
	}

	node->type = type;
	node->next = 0;

	if (owner) {
		node->owner = owner;
	} else {
		node->owner = 0;
	}

	if (mac) {
		strncpy(node->mac, mac, 6);
	} else {
		node->name[0] = '\0';
	}

	if (name) {
		strncpy(node->name, name, IRC_BUFFER_SIZE);
		node->name[IRC_BUFFER_SIZE-1] = '\0';
	} else {
		node->name[0] = '\0';
	}

	irc_last_node = node;

	return node;
}

irc_node * irc_destroy_node(irc_node * node) {
	irc_node * aux;
	if (node == 0 || irc_root_node == 0 || irc_last_node == 0) {
		return irc_error_object_not_found("data node");
	} else if (node == irc_root_node) {
		aux = irc_root_node->next;
		free(irc_root_node);
		irc_root_node = aux;
		return 1;
	} else {
		aux = _irc_search_node_by_next(node);
		aux->next = node->next;
		if (node == irc_last_node) {
			irc_last_node = aux;
		}
		free(node);
		return 1;
	}
	return 0;
}