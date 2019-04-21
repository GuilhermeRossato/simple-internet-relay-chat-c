#include "irc_shared.h"
#include "irc_put_ethernet_interface_address_by_name.c"
#include "irc_put_ethernet_interface_name_by_id.c"
#include "irc_data.c"
#include "irc_user.c"
#include "irc_channel.c"
#include "irc_listen.c"
#include "irc_send.c"

// IRC Interface

int irc_on_message_received(char * origin, char * message);

// IRC Implementation

int irc_is_command(char * message, char * command) {
	if (message[0] != '/') {
		return 0;
	}
	if (_irc_compare_two_strings(message+1, command, strlen(command))) {
		return 1;
	}
	return 0;
}

char * irc_get_command_parameter(char * message, char * command) {
	return message + strlen(command) + 1;
}

int irc_cmd_join_channel(char * origin, char * channel_name) {
	printf("[Joining channel \"%s\"]\n", channel_name);
	if (!irc_check_channel_exists(channel_name)) {
		return irc_error_object_not_found("channel");
	}
	if (irc_check_listen_by_origin_and_name(origin, channel_name)) {
		return irc_error_object_already_exists("user subscription");
	}
	if (!irc_create_listen(origin, channel_name)) {
		return 0;
	}
	return 1;
}

int irc_cmd_exit_channel(char * origin, char * channel_name) {
	printf("[Exiting channel \"%s\"]\n", channel_name);
	if (!irc_check_channel_exists(channel_name)) {
		return irc_error_object_not_found("channel");
	}
	if (!irc_check_listen_by_origin_and_name(origin, channel_name)) {
		return irc_error_object_not_found("user subscription");
	}
	if (!irc_destroy_listen_by_origin_and_name(origin, channel_name)) {
		return 0;
	}
	return 1;
}

int irc_cmd_create_channel(char * origin, char * channel_name) {
	printf("[Creating channel \"%s\"]\n", channel_name);
	if (irc_check_channel_exists(channel_name)) {
		return irc_error_object_already_exists("channel");
	}
	if (!irc_create_channel(origin, channel_name)) {
		return 0;
	}
	return 1;
}

int irc_cmd_exit_user(char * origin) {
	while (irc_check_channel_by_origin(origin)) {
		if (!irc_destroy_channel_by_origin(origin)) {
			return 0;
		}
	}
	while (irc_check_user_by_origin(origin)) {
		if (!irc_destroy_user_by_origin(origin)) {
			return 0;
		}
	}
	while (irc_check_listen_by_origin(origin)) {
		if (!irc_destroy_listen_by_origin(origin)) {
			return 0;
		}
	}
}

int irc_on_message_received(char * origin, char * message) {
	char * params;
	int success;

	// Add that person to user list if it does not exist
	if (!irc_check_user_by_origin(origin)) {
		printf("User haven't authenticated\n");
		return 0;
	}

	if (message[0] != '\\' && message[0] != '\0') {
		printf("Broadcast\n");
		return 1;
	} else if (irc_is_command(message, "join")) {
		return irc_cmd_join_channel(origin, irc_get_command_parameter(message, "join"));
	} else if (irc_is_command(message, "create")) {
		return irc_cmd_create_channel(origin, irc_get_command_parameter(message, "create"));
	} else if (irc_is_command(message, "leave")) {
		return irc_cmd_exit_channel(origin, irc_get_command_parameter(message, "leave"));
	} else if (irc_is_command(message, "exit")) {
		return irc_cmd_exit_user(origin);
	} else {
		return irc_error_object_not_found("specified command");
	}
}