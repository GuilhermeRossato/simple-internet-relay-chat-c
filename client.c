#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include "src/irc.h"

int user_input_index = 0;
int user_input_length = 0;
char user_input[128];
int is_input_insert = 0;
int flashing_indication = 0;

void print_date() {
	time_t t = time(NULL);
	struct tm tm = *localtime(&t);
	printf("%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
}

void update_screen() {
	gotoxy(0, 0);
	// Header
	printf("| IRC Client | Time: ");
	print_date();
	printf("                           ");

	// Content
	int i;
	int height = irc_get_console_height();
	for (i=0;i<=10;i++) {
		if (i+1 >= height) {
			break;
		}
		gotoxy(0, i+2);
		if (i == 10) {
			printf("                                                ");
		} else {
			printf("%s\n", screen_data[i]);
		}
	}
	printf("\n");

	// User input
	gotoxy(0, height-1);
	printf("                                                      ");
	gotoxy(0, height-1);
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
		}
		putchar(user_input[i]);
	}
	printf(" %d\n", user_input_index);
}

void update_loop() {
	if (is_program_finished == 1) {
		exit(1);
	}
	flashing_indication = flashing_indication == 0 ? 1 : 0;
	update_screen();
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
		letter == ']');
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
	for (int i = 1 ; i < 30; i++) {
		snprintf(pdt->screen_data[i-1], 127, "%s", pdt->screen_data[i]);
	}
	snprintf(pdt->screen_data[9], 127, "%s", message);
}

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
	int screen_data_index;
	char screen_data[128][30] = {0};
} pipe_data_type;


int handle_input(pipe_data_type * pdt) {
	int input;
	while(1) {
		if (is_program_finished == 1) {
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
			irc_send(user_input, pdt->interface_name, pdt->origin_mac, pdt->origin_ip, pdt->target_mac, pdt->target_ip);

			// clear user input
			user_input_index = 0;
			user_input_length = 0;
			user_input[0] = '\0';
			update_screen();
		} else if (is_character_acceptable(input)) {
			add_character_to_input(input);
			update_screen();
		} else {
			printf("\n%d\n", input);
		}
	}
}

int receive(char * message, int message_length, void * origin) {
	if (irc_string_equal(message, "/terminate-client")) {
		irc_stop_server();
		printf("The client was terminated remotely\n");
	}
}

int handle_server(pipe_data_type * pdt) {
	irc_server(pdt->interface_name, pdt->origin_mac, receive);
	pdt->is_program_finished = 1;
}

pipe_data_type * pdt;

int server_identified_itself(char * message, int message_length, void * v_origin) {
	message_data_type * origin = (message_data_type *) v_origin;
	printf(
		"Got server MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
		origin->origin_mac[0],
		origin->origin_mac[1],
		origin->origin_mac[2],
		origin->origin_mac[3],
		origin->origin_mac[4],
		origin->origin_mac[5]
	);
	memcpy(pdt->target_mac, origin->origin_mac, 6);
	pdt->target_mac[6] = '\0';
	irc_stop_server();
}


int main(int argn, char ** argc) {

	pipe_data_type pdt_original;

	pdt = &pdt_original;
	pdt->is_program_finished = 0;
	pdt->interface_id = -1;
	pdt->send_message_buffer = 0;
	pdt->screen_data_index = 0;
	snprintf(pdt.username, 32, "unnamed");

	while (1) {
		select_interface(&(pdt->interface_id));
		printf("\n");
		while (1) {
			select_mac("origin MAC", pdt->origin_mac, 32);
			if (strncmp(pdt->origin_mac, "FF:FF:FF:FF:FF:FF", 32) == 0) {
				printf("Origin MAC cannot be broadcast\n");
				continue;
			}
			break;
		}
		printf("\n");
		select_ip("origin IP", pdt->origin_ip, 32);
		printf("\n");
		select_mac("target MAC", pdt->target_mac, 32);
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

	irc_put_ethernet_interface_name_by_id(pdt->interface_id, pdt->interface_name, 32);

	pthread_t listener_thread;

	if(pthread_create(&listener_thread, NULL, handle_server, &pdt)) {
		fprintf(stderr, "Error creating thread\n");
		return 1;
	}

	clrscr();

	if (strncmp(pdt->target, "FF:FF:FF:FF:FF:FF", 32) == 0) {
		printf("Finding server by sending broadcast...\n");
		// find the server by broadcasting <who is server> packet
		irc_send("/who-is-server", pdt->interface_name, pdt->origin_mac, pdt->origin_ip, pdt->target_mac, pdt->target_ip);
		irc_server(pdt->interface_name, pdt->origin_mac, server_identified_itself);
	} else {
		printf("Loading...\n");
	}

	irc_start_timer(update_loop);
	handle_input(&pdt);
	pdt->is_program_finished = 1;
	pthread_join(&listener_thread, NULL);
	return 0;
}