#include "./src/irc.h"

typedef struct pipe_data_type {
	int interface_id;
	char interface_name[32];
	char origin_mac[32];
} pipe_data_type;

pipe_data_type * pdt;

int reply(char * message, message_data_type * origin) {
	return irc_send(
		message,
		pdt->interface_name,
		pdt->origin_mac,
		origin->target_ip,
		origin->origin_mac,
		origin->origin_ip
	);
}

int apply_broadcast(message_data_type * origin, char * message) {
	if (message[0] != '<') {
		// Do not send broadcast without an username
		return 0;
	}
	int user_id = 0;
	char * origin_string;
	char target_mac[IRC_BUFFER_SIZE];
	char target_ip[IRC_BUFFER_SIZE];
	printf("Sending broadcast message '%s'\n", message);
	do {
		origin_string = irc_get_user_origin_by_id(user_id++);
		if (origin_string == 0) {
			break;
		}
		irc_split_string(origin_string, '|', target_mac, target_ip, IRC_BUFFER_SIZE);
		irc_send(
			message,
			pdt->interface_name,
			pdt->origin_mac,
			origin->target_ip,
			target_mac,
			target_ip
		);
	} while (origin_string != 0);
}

int validate_user_or_create(char * origin_string) {
	if (!irc_check_user_by_origin(origin_string)) {
		char user_name[IRC_BUFFER_SIZE];
		snprintf(user_name, IRC_BUFFER_SIZE, "unnamed%d", 1+(rand()%99));
		printf("Adding new user \"%s\" from %s\n", user_name, origin_string);
		irc_create_user(origin_string, user_name);
	}
}

int receive(char * message, int message_length, void * v_origin) {
	message_data_type * origin = (message_data_type *) v_origin;

	if (irc_compare_two_strings(message, "/who-is-server", message_length)) {
		return reply("/i-am", origin);
	}

	char origin_string[IRC_BUFFER_SIZE];
	irc_put_origin_by_mac_and_ip(origin->origin_mac, origin->origin_ip, origin_string, IRC_BUFFER_SIZE);

	// Assert the user exists, creating it if it does not.
	validate_user_or_create(origin_string);

	// Handle broadcast
	if (message[0] != '/') {
		char * user_name = irc_get_user_name_by_origin(origin_string);
		char message_with_name[IRC_BUFFER_SIZE+32];
		if (user_name == 0) {
			snprintf(message_with_name, IRC_BUFFER_SIZE+32, "<Unknown> %s", message);
		} else {
			snprintf(message_with_name, IRC_BUFFER_SIZE+32, "<%s> %s", user_name, message);
		}
		return apply_broadcast(origin, message_with_name);
	}

}

int select_interface(int * output) {
	char interface_name[64];
	char interface_address[64];
	char binary_mac[7];
	int count = 0;
	char selection;
	int id = 0;
	while (irc_put_ethernet_interface_name_by_id(count, interface_name, 64)) {
		if (count == 0) {
			printf("Select an interface:\n\n");
		}
		printf("[%d] %s\n", count, interface_name);
		count++;
	}
	if (count == 0) {
		printf("There are no active internet interfaces\n");
		*output = 0;
		return 0;
	}
	printf("\n\n");
	while (1) {
		selection = getch();
		if (selection < '0' || selection > '9') {
			printf("Press a number that correspond to an interface to select it.\n");
			continue;
		}
		id = selection-'0';
		if (!irc_put_ethernet_interface_name_by_id(id, interface_name, 64)) {
			printf("Invalid option: %c\n", selection);
			continue;
		}
		interface_address[0] = '\0';
		if (!irc_put_ethernet_interface_address_by_name(interface_name, binary_mac, 7)) {
			snprintf(interface_address, 64, "?");
		} else {
			snprintf(interface_address, 64, "%02X:%02X:%02X:%02X:%02X:%02X", binary_mac[0] & 0xFF, binary_mac[1] & 0xFF, binary_mac[2] & 0xFF, binary_mac[3] & 0xFF, binary_mac[4] & 0xFF, binary_mac[5] & 0xFF);
		}
		printf(" > Interface: \"%s\" [%s]\n", interface_name, interface_address);
		*output = id;
		break;
	}
}

int select_mac(char * mac_name, char * output, int output_length) {
	char read_buffer[256];
	printf("Select a %s (or empty for default FF:FF:FF:FF:FF:FF)\n", mac_name);
	fgets(read_buffer, 256, stdin);
	int i, j, is_separator, is_number, is_letter;
	j = 0;
	if (output_length > 0) {
		output[0] = '\0';
	}
	for (i=0;i<256;i++) {
		if (j > output_length) {
			break;
		}
		if (read_buffer[i] == '\0') {
			output[j++] = '\0';
			break;
		}
		is_separator = (read_buffer[i] == ':');
		is_number = (read_buffer[i] >= '0' && read_buffer[i] <= '9');
		is_letter = ((read_buffer[i] >= 'a' && read_buffer[i] <= 'f') || (read_buffer[i] >= 'A' && read_buffer[i] <= 'F'));
		if (!is_separator && !is_number && !is_letter) {
			continue;
		}
		output[j++] = read_buffer[i];
	}
	if (output[0] == '\0') {
		snprintf(output, output_length, "FF:FF:FF:FF:FF:FF");
	} else if (j < output_length) {
		output[j++] = '\0';
	}
	printf(" > MAC: %s\n", output);
}

int main(int argn, char ** argc) {
	pipe_data_type pdt_original;

	pdt = &pdt_original;
	pdt->interface_id = -1;

	while (1) {
		select_interface(&pdt->interface_id);
		printf("\n");
		select_mac("MAC", pdt->origin_mac, 32);
		printf("\n");
		printf("Are you sure the selected values are correct? [y/n] ");
		char confirm = getch();
		printf("\n");
		if (confirm == 'y' || confirm == 'Y' || confirm == 's' || confirm == 'S') {
			break;
		}
	}

	printf("Listening for messages...\n");
	irc_put_ethernet_interface_name_by_id(pdt->interface_id, pdt->interface_name, 64);

	if (!irc_server(pdt->interface_name, pdt->origin_mac, receive)) {
		return 1;
	}
	return 0;
}
