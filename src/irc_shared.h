#include <string.h>

#ifndef IRC_SHARED
#define IRC_SHARED

#define IRC_BUFFER_SIZE	64

enum irc_node_type {
	irc_none = 0,
	irc_user,
	irc_listen,
	irc_channel
};

typedef struct irc_node {
	enum irc_node_type type;
	int owner;
	char origin[IRC_BUFFER_SIZE];
	char name[IRC_BUFFER_SIZE];
	struct irc_node * next;
} irc_node;

irc_node * irc_root_node = 0;
irc_node * irc_last_node = 0;

int _irc_compare_two_strings(char * a, char * b, int max_length) {
	if (a == b) {
		return 1;
	} else if (a == 0 || b == 0) {
		return 0;
	} else if (strncmp(a, b, max_length) == 0) {
		return 1;
	}
	return 0;
}

int irc_error_object_already_exists(char * object_name) {
	printf("IRC Error: The %s already exists\n", object_name);
	return 0;
}
int irc_error_invalid_something(char * something) {
	printf("IRC Error: The %s is invalid\n", something);
	return 0;
}
int irc_error_object_not_found(char * object_name) {
	printf("IRC Error: The %s was not found\n", object_name);
	return 0;
}
int irc_error_malloc_failed(char * context) {
	printf("IRC Error: Malloc failed at %s\n", context);
	return 0;
}
int irc_error_could_not(char * thing) {
	printf("IRC Error: Could not %s due to an error result", thing);
	return 0;
}
#endif