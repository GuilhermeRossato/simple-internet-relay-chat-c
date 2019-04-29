#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "src/irc.h"

int user_input_index = 0;
int user_input_length = 0;
char last_user_input[128];
char user_input[128];
int is_input_insert = 0;
int flashing_indication = 0;

#define SCREEN_DATA_LINE_COUNT	15

typedef struct pipe_data_type {
	int is_program_finished;
	int interface_id;
	int send_message_buffer;
	char username[32];
	char message_buffer[128];
	char interface_name[32];
	char origin_mac[32];
	char origin_ip[32];
	char target_mac[32];
	char target_ip[32];
	char screen_data[128][SCREEN_DATA_LINE_COUNT];
	int screen_data_index;
	int just_skip;
	int is_waiting_server_origin;
} pipe_data_type;

pipe_data_type * pdt;

void print_date() {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	printf("%02d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void update_screen() {
	if (pdt->is_waiting_server_origin == 1) {
		return;
	}
	//clrscr();
	// Header
	printf("\n");
	printf("| IRC Client | Time: ");
	print_date();
	printf("\nClient Address: ");
	if (_irc_is_string_mac(pdt->origin_mac)) {
		printf("%s", pdt->origin_mac);
	} else {
		printf(
			"%02X:%02X:%02X:%02X:%02X:%02X",
			pdt->origin_mac[0] & 0xFF,
			pdt->origin_mac[1] & 0xFF,
			pdt->origin_mac[2] & 0xFF,
			pdt->origin_mac[3] & 0xFF,
			pdt->origin_mac[4] & 0xFF,
			pdt->origin_mac[5] & 0xFF
		);
	}
	printf(" | Server Address: ");
	if (_irc_is_string_mac(pdt->target_mac)) {
		printf("%s", pdt->target_mac);
	} else {
		printf(
			"%02X:%02X:%02X:%02X:%02X:%02X",
			pdt->target_mac[0] & 0xFF,
			pdt->target_mac[1] & 0xFF,
			pdt->target_mac[2] & 0xFF,
			pdt->target_mac[3] & 0xFF,
			pdt->target_mac[4] & 0xFF,
			pdt->target_mac[5] & 0xFF
		);	
	}
	printf("\n");

	// Content
	int i;
	int height = irc_get_console_height();

	for (i=0;i<=SCREEN_DATA_LINE_COUNT;i++) {
		if (i+1 >= height) {
			break;
		}
		if (i == SCREEN_DATA_LINE_COUNT) {
			printf("                                          \n");
		} else {
			printf("%s\n", pdt->screen_data[i]);
		}
	}

	printf("\n");

	// User input
	printf("\n\n");
	printf(" > ");

	for(i=0;i<user_input_length;i++) {
		if (i>127) {
			break;
		}
		if (user_input_index != i && user_input[i] == '\0') {
			break;
		}
		if (i == user_input_index && flashing_indication == 1) {
			putchar('_');
			continue;
		}
		putchar(user_input[i]);
	}
	printf("  \n");
}

void update_loop() {
	if (pdt->is_waiting_server_origin) {
		if (!irc_send("/who-is-server", pdt->interface_name, pdt->origin_mac, pdt->origin_ip, pdt->target_mac, pdt->target_ip)) {
			printf("Failed sending 'who is server' packet\n");
		}
	}
	if (pdt->is_program_finished == 1) {
		exit(1);
	}
	if (pdt->just_skip == 0) {
		flashing_indication = flashing_indication == 0 ? 1 : 0;
		update_screen();
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

int select_mac(char * mac_name, char * output, int output_length, char * default_mac) {
	char read_buffer[256];
	char actual_default_mac[32];
	if (default_mac != 0) {
		snprintf(actual_default_mac, 32, "%s", default_mac);
	} else {
		snprintf(actual_default_mac, 32, "FF:FF:FF:FF:FF:FF");
	}
	printf("Select a %s (or empty for default %s)\n", mac_name, actual_default_mac);
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
		snprintf(output, output_length, "%s", actual_default_mac);
	} else if (j < output_length) {
		output[j++] = '\0';
	}
	printf(" > MAC: %s\n", output);
}

int select_ip(char * ip_name, char * output, int output_length) {
	char read_buffer[256];
	printf("Select a %s (or empty for default 127.0.0.1)\n", ip_name);
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
		is_separator = (read_buffer[i] == '.');
		is_number = (read_buffer[i] >= '0' && read_buffer[i] <= '9');
		if (!is_separator && !is_number) {
			continue;
		}
		output[j++] = read_buffer[i];
	}
	if (output[0] == '\0') {
		snprintf(output, output_length, "127.0.0.1");
	} else if (j < output_length) {
		output[j++] = '\0';
	}
	printf(" > IP: %s\n", output);
}

int is_character_acceptable(char letter) {
	int is_separator = (
		letter == ':' ||
		letter == ';' ||
		letter == '.' ||
		letter == ',' ||
		letter == ' ' ||
		letter == '-' ||
		letter == '+' ||
		letter == '*' ||
		letter == '/' ||
		letter == '\\' ||
		letter == '@' ||
		letter == '=' ||
		letter == '!' ||
		letter == '(' ||
		letter == ')' ||
		letter == '[' ||
		letter == ']' ||
		letter == ' ');
	int is_number = (letter >= '0' && letter <= '9');
	int is_letter = ((letter >= 'a' && letter <= 'z') || (letter >= 'A' && letter <= 'Z'));

	return is_separator || is_number || is_letter;
}

int add_character_to_input(char letter) {
	// add on empty text or at the end
	if (user_input_length == 0 || user_input_index == user_input_length) {
		user_input[user_input_index++] = letter;
		user_input[user_input_index] = '\0';
		user_input_length = user_input_index;
		return 1;
	}
	// add at middle
	if (is_input_insert == 1) {
		user_input[user_input_index] = letter;
		user_input_index++;
	} else {
		int i;
		for(i=user_input_index;i<127;i++) {
			user_input[i+1] = user_input[i];
			if (user_input[i] == '\0') {
				break;
			}
		}
		user_input[user_input_index] = letter;
		user_input_index++;
		user_input_length++;
	}
}


int add_unimplemented_to_input() {
	add_character_to_input('(');
	add_character_to_input('n');
	add_character_to_input('o');
	add_character_to_input('t');
	add_character_to_input(' ');
	add_character_to_input('i');
	add_character_to_input('m');
	add_character_to_input('p');
	add_character_to_input('l');
	add_character_to_input('e');
	add_character_to_input('m');
	add_character_to_input('e');
	add_character_to_input('n');
	add_character_to_input('t');
	add_character_to_input('e');
	add_character_to_input('d');
	add_character_to_input(')');
}

int add_line_to_screen(pipe_data_type * pdt, char * message) {
	for (int i = 1 ; i < SCREEN_DATA_LINE_COUNT; i++) {
		snprintf(pdt->screen_data[i-1], 127, "%s", pdt->screen_data[i]);
	}
	snprintf(pdt->screen_data[SCREEN_DATA_LINE_COUNT-1], 127, "%s", message);
}


int handle_input(pipe_data_type * pdt) {
	int input;
	while(1) {
		if (pdt->is_program_finished == 1) {
			exit(0);
		}
		input = getch();

		// convert c with trail to simple c
		if (input == 195) {
			input = getch();
			if (input == 167) {
				input = 'c';
			}
		}

		if (input == 127) {
			// backspace
			if (user_input_index < 0) {
				continue;
			}
			if (user_input_index == user_input_length) {
				user_input[user_input_index] = '\0';
				user_input_index--;
				user_input_length = user_input_index;
			} else {
				snprintf(user_input+user_input_index, 127-user_input_index, "%s", user_input+user_input_index+1);
				user_input_index--;
				user_input_length--;
			}
			update_screen();
		} else if (input == 20) {
			// pasting
			add_unimplemented_to_input();
		} else if (input == 27) {
			// arrow key indicator
			input = getch();
			if (input == '[') {
				input = getch();
			}
			if (input == 'D') {
				if (user_input_index > 0) {
					user_input_index--;
					update_screen();
				}
			} else if (input == 'C') {
				if (user_input_index < user_input_length && user_input_index < 127) {
					user_input_index ++;
					update_screen();
				}
			}
		} else if (input == 50) {
			is_input_insert = !is_input_insert;
		} else if (input == 10) {
			// enter (send)
			// send to server
			snprintf(last_user_input, 128, "%s", user_input);
			last_user_input[127] = '\0';
			if (last_user_input[0] != '\0' && last_user_input[1] != '\0') {
				irc_send(last_user_input, pdt->interface_name, pdt->origin_mac, pdt->origin_ip, pdt->target_mac, pdt->target_ip);
			}

			// clear user input
			user_input_index = 0;
			user_input_length = 0;
			user_input[0] = '\0';
			//update_screen();
		} else if (is_character_acceptable(input)) {
			add_character_to_input(input);
			update_screen();
		} else {
			printf("\n%d\n", input);
		}
	}
}

int receive(char * message, int message_length, void * origin) {
	printf("Client received %d bytes: %s\n", message_length, message);
	if (irc_compare_two_strings(message, "/terminate-client", message_length)) {
		irc_stop_server();
		printf("The client was terminated remotely\n");
	}
}

void * handle_server(void * pdt_v) {
	pipe_data_type * pdt = (pipe_data_type *) pdt_v;
	irc_server(pdt->interface_name, pdt->origin_mac, receive);
	pdt->is_program_finished = 1;
}

int server_identified_itself(char * message, int message_length, void * v_origin) {
	if (message[0] != '/' || message[1] != 'i' || message[3] != 'a' || message[4] != 'm') {
		printf("Ignored invalid who is server packet\n");
		return 0;
	}
	message_data_type * origin = (message_data_type *) v_origin;
	printf(
		"Server replied from MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
		origin->origin_mac[0] & 0xFF,
		origin->origin_mac[1] & 0xFF,
		origin->origin_mac[2] & 0xFF,
		origin->origin_mac[3] & 0xFF,
		origin->origin_mac[4] & 0xFF,
		origin->origin_mac[5] & 0xFF
	);
	pdt->is_waiting_server_origin = 0;
	memcpy(pdt->target_mac, origin->origin_mac, 6);
	pdt->target_mac[6] = '\0';
	irc_stop_server();
}

void read_input_for_addresses() {
	while (1) {
		select_interface(&(pdt->interface_id));
		printf("\n");
		while (1) {
			char default_mac[32];
			snprintf(default_mac, 32, "FF:FF:FF:FF:%02X:%02X", rand() % 255, rand() % 255);
			select_mac("origin MAC", pdt->origin_mac, 32, default_mac);
			if (strncmp(pdt->origin_mac, "FF:FF:FF:FF:FF:FF", 32) == 0) {
				printf("Origin MAC cannot be broadcast\n");
				continue;
			}
			break;
		}
		printf("\n");
		select_ip("origin IP", pdt->origin_ip, 32);
		printf("\n");
		select_mac("target MAC", pdt->target_mac, 32, 0);
		printf("\n");
		select_ip("target IP", pdt->target_ip, 32);
		printf("\n");
		printf("Are you sure the selected values are correct? [y/n] ");
		char confirm = getch();
		printf("\n");
		if (confirm == 'y' || confirm == 'Y' || confirm == 's' || confirm == 'S') {
			break;
		}
	}
}

void set_default_input_for_addresses() {
	pdt->interface_id = 0;
	snprintf(pdt->origin_mac, 32, "%02X:FF:FF:FF:%02X:%02X", rand() % 255, rand() % 255, rand() % 255);
	snprintf(pdt->origin_ip, 32, "127.0.0.1");
	snprintf(pdt->target_mac, 32, "FF:FF:FF:FF:FF:FF");
	snprintf(pdt->target_ip, 32, "127.0.0.1");
}

int main(int argn, char ** argc) {

	pipe_data_type pdt_original;

	pdt = &pdt_original;
	pdt->is_program_finished = 0;
	pdt->interface_id = -1;
	pdt->send_message_buffer = 0;
	pdt->screen_data_index = 0;
	pdt->is_waiting_server_origin = 0;

	snprintf(pdt->username, 32, "unnamed");
	for (int i = 0 ; i < SCREEN_DATA_LINE_COUNT; i++) {
		pdt->screen_data[i][0] = '\0';
	}

	pdt->just_skip = 1;
	irc_start_timer(update_loop);

	if (argn == 2 && argc[1][0] == '-' && argc[1][1] == 'y') {
		set_default_input_for_addresses();
	} else {
		read_input_for_addresses();
	}
	pdt->just_skip = 0;

	irc_put_ethernet_interface_name_by_id(pdt->interface_id, pdt->interface_name, 32);

	if (strncmp(pdt->target_mac, "FF:FF:FF:FF:FF:FF", 32) == 0) {
		printf("Finding server by sending broadcast...\n");
		pdt->is_waiting_server_origin = 1;
		irc_server(pdt->interface_name, pdt->origin_mac, server_identified_itself);
	} else {
		printf("Loading...\n");
	}

	pthread_t listener_thread;

	if(pthread_create(&listener_thread, NULL, handle_server, pdt)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	handle_input(pdt);
	pdt->is_program_finished = 1;
	pthread_join(listener_thread, NULL);
	return 0;
}
