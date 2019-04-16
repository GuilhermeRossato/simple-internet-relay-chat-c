#include "irc_shared.h";

#include "irc_data.c";
#include "irc_channel.c";
#include "irc_user.c";
#include "irc_listen.c";
#include "irc_server.c";

int irc_join_channel(char * origin_mac, char * channel_name) {
	printf("[Joining channel \"%s\"]\n", channel_name);
	if (!irc_check_channel_exists(channel_name)) {
		return irc_error_object_not_found("channel");
	}
	if (irc_check_listen_by_mac_and_name(origin_mac, channel_name)) {
		return irc_error_object_already_exists("user subscription");
	}
	if (!irc_create_listen(origin_mac, channel_name)) {
		return 0;
	}
	return 1;
}

int irc_exit_channel(char * origin_mac, char * channel_name) {
	printf("[Exiting channel \"%s\"]\n", channel_name);
	if (!irc_check_channel_exists(channel_name)) {
		return irc_error_object_not_found("channel");
	}
	if (!irc_check_listen_by_mac_and_name(origin_mac, channel_name)) {
		return irc_error_object_not_found("user subscription");
	}
	if (!irc_destroy_listen_by_mac_and_name(origin_mac, channel_name)) {
		return 0;
	}
	return 1;
}

int irc_broadcast_message(char * origin_mac, char * message) {

}

int irc_create_channel(char * origin_mac, char * channel_name) {
	printf("[Creating channel \"%s\"]\n", channel_name);
	if (irc_check_channel_exists(channel_name)) {
		return irc_error_object_already_exists("channel");
	}
	if (!irc_create_channel(origin_mac, channel_name)) {
		return 0;
	}
	return 1;
}

int irc_exit_user(char * origin_mac) {
	while (irc_check_channel_by_mac(origin_mac)) {
		if (!irc_destroy_channel_by_mac(origin_mac)) {
			return 0;
		}
	}
	while (irc_check_user_by_mac(origin_mac)) {
		if (!irc_destroy_user_by_mac(origin_mac)) {
			return 0;
		}
	}
	while (irc_check_listen_by_mac(origin_mac)) {
		if (!irc_destroy_listen_by_mac(origin_mac)) {
			return 0;
		}
	}
}

int irc_on_message_received(char * origin_mac, char * message) {
	char * params;
	int success;

	// Add that person to user list if it does not exist
	if (!irc_check_user_by_mac(origin_mac)) {
		success = irc_create_user(origin_mac);
		if (!success) {
			return 0;
		}
	}

	if (message[0] != '\0') {
		return irc_broadcast_message(origin_mac, message);
	} else if (irc_is_command(message, "join")) {
		return irc_join_channel(origin_mac, irc_get_command_parameter(message, "join"));
	} else if (irc_is_command(message, "create")) {
		return irc_create_channel(origin_mac, irc_get_command_parameter(message, "create"));
	} else if (irc_is_command(message, "leave")) {
		return irc_leave_channel(origin_mac, irc_get_command_parameter(message, "leave"));
	} else if (irc_is_command(message, "exit")) {
		return irc_exit_user(origin_mac);
	} else {
		return irc_error_object_not_found("specified command");
	}
}

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