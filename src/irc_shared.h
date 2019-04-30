#ifndef IRC_SHARED
#define IRC_SHARED

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define IRC_BUFFER_SIZE	128

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

int irc_compare_two_strings(char * a, char * b, int max_length) {
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
	printf("IRC Error: Could not %s due to an error result\n", thing);
	return 0;
}

int _irc_is_string_mac(char * mac) {
	int i, is_separator, is_number, is_letter;
	for (i=0;i<6;i++) {
		is_separator = (mac[i] == ':');
		is_number = (mac[i] >= '0' && mac[i] <= '9');
		is_letter = ((mac[i] >= 'a' && mac[i] <= 'f') || (mac[i] >= 'A' && mac[i] <= 'F'));

		if (i == 0 && !is_letter && !is_number) {
			return 0;
		}

		if ((i == 2 || i == 5) && !is_separator) {
			return 0;
		}

		if (!is_number && !is_letter && !is_separator) {
			return 0;
		}
	}
	return 1;
}

int irc_split_string(char * original_string, char separator, char * left_buffer, char * right_buffer, int individual_buffer_size) {
	int i;
	int j = 0;
	int left_buffer_index = 0;
	int right_buffer_index = 0;
	for (i=0;i < individual_buffer_size*2; i++) {
		if (j == 0) {
			if (original_string[i] == '\0') {
				left_buffer[left_buffer_index++] = '\0';
				right_buffer[0] = '\0';
				break;
			} else if (original_string[i] == separator) {
				left_buffer[left_buffer_index++] = '\0';
				j = 1;
			} else {
				if (left_buffer_index >= individual_buffer_size-1) {
					left_buffer[individual_buffer_size-1] = '\0';
				} else {
					left_buffer[left_buffer_index++] = original_string[i];
				}
			}
		} else {
			if (right_buffer_index >= individual_buffer_size-1) {
				right_buffer[individual_buffer_size-1] = '\0';
			} else {
				right_buffer[right_buffer_index++] = original_string[i];
			}
			if (original_string[i] == '\0') {
				break;
			}
		}
	}
	return 1;
}

int _irc_is_string_ip(char * ip) {
	int i, is_separator, is_number;
	for (i=0;i<4;i++) {
		is_separator = (ip[i] == '.');
		is_number = (ip[i] >= '0' && ip[i] <= '9');
		if (i == 0 && !is_number) {
			return 0;
		}
		if (!is_number && !is_separator) {
			return 0;
		}
	}
	return 1;
}

int _irc_mac_to_binary(char * macStr, uint8_t * binary_mac) {
	if (!_irc_is_string_mac(macStr)) {
		memcpy(binary_mac, macStr, 6);
		return 1;
	}
	unsigned char mac[6];
	sscanf(macStr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	binary_mac[0] = mac[0];
	binary_mac[1] = mac[1];
	binary_mac[2] = mac[2];
	binary_mac[3] = mac[3];
	binary_mac[4] = mac[4];
	binary_mac[5] = mac[5];

	return 1;
}

int _irc_ip_to_binary(char * ip, uint8_t * binary_ip) {
	if (!_irc_is_string_ip(ip)) {
		memcpy(binary_ip, ip, 6);
		return 1;
	}
	sscanf(ip,"%d.%d.%d.%d", (unsigned int *) &binary_ip[0], (unsigned int *) &binary_ip[1], (unsigned int *) &binary_ip[2], (unsigned int *) &binary_ip[3]);
	return 1;
}

#endif
