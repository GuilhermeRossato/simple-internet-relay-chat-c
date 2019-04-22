#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "src/irc_conio.c"
#include "src/irc_timer.c"
#include "src/irc_put_ethernet_interface_name_by_id.c"
#include "src/irc_put_ethernet_interface_address_by_name.c"

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

int screen_data_index = 0;
char screen_data[128][10] = {0};

void update_screen() {
	gotoxy(0, 0);
	// Header
	printf("| IRC Client | Time: ");
	print_date();

	// Content
	int i;
	int height = irc_get_console_height();
	for (i=0;i<=10;i++) {
		if (i+1 >= height) {
			break;
		}
		gotoxy(0, i);
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
	putchar('\n');
}

void update_loop() {
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

int select_mac(char * output, int output_length) {
	char read_buffer[256];
	printf("Select a MAC (or empty for default FF:FF:FF:FF:FF:FF)\n");
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

int select_ip(char * output, int output_length) {
	char read_buffer[256];
	printf("Select a IP (or empty for default 127.0.0.1)\n");
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

int main(int argn, char ** argc) {

	int interface_id;
	char mac[32];
	char ip[32];

	while (1) {
		select_interface(&interface_id);
		select_mac(mac, 32);
		select_ip(ip, 32);
		printf("Are you sure the selected values are correct? [y/n] ");
		char confirm = getch();
		printf("\n");
		if (confirm == 'y' || confirm == 'Y' || confirm == 's' || confirm == 'S') {
			break;
		}
	}

	clrscr();

	irc_start_timer(update_loop);

	printf("Loading...");

	int input;
	while(1) {
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
			if (user_input_index == user_input_length) {
				user_input[user_input_index] = '\0';
				user_input_index--;
				user_input_length = user_input_index;
			} else {
				//snprintf(userdata+user_input_index, 127-user_input_index, "%s", userdata+user_input_index+1);
				//user_input_index--;
				//user_input_length--;
				printf("\nNot supported\n");
			}
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
				}
			} else if (input == 'C') {
				if (user_input_index < user_input_length && user_input_index < 127) {
					user_input_index ++;
				}
			}
			update_screen();
		} else if (input == 50) {
			is_input_insert = !is_input_insert;
		} else if (input == 10) {
			// enter (send)
			// TODO send data with IRC_SEND
			if (screen_data_index < 10){
				screen_data_index++;
			}
			snprintf(screen_data[screen_data_index], 126, "<unammed> %s", user_input);
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

	return 0;
}