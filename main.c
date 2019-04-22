#include <stdlib.h>
#include <stdio.h>
#include "src/irc_conio.c"
#include "src/irc_timer.c"
#include "src/irc_put_ethernet_interface_name_by_id.c"
#include "src/irc_put_ethernet_interface_address_by_name.c"

int user_input_index = 0;
char user_input[128];

void update_screen() {
	printf("Update_screend\n");
}

void select_interface(int * output) {
	char interface_name[64];
	char interface_address[64];
	int count;
	char selection;
	int id = 0;
	printf("Select an interface:\n");
	snprintf(interface_name, 64, "enlo");
	while (irc_put_ethernet_interface_name_by_id(count, interface_name, 64)) {
		printf("[%d] %s\n", count, interface_name);
		count++;
	}
	printf("\n");
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
		if (!irc_put_ethernet_interface_address_by_name(interface_name, interface_address, 64)) {
			interface_address[0] = '?';
			interface_address[1] = '\0';
		}
		printf("Selected interface: \"%s\" [%s]\n", interface_name, interface_address);
		*output = id;
		break;
	}
}

void select_mac(char * output, int output_length) {
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
	printf("Selected mac: %s\n", output);
}

void select_ip(char * output, int output_length) {
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
	printf("Selected ip: %s\n", output);
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
		if (confirm == 'y' || confirm == 'Y' || confirm == 's' || confirm == "S") {
			break;
		}
	}

	irc_start_timer(update_screen);

	//clrscr();
	//gotoxy(0, 0);
	printf("Chat System - Last Packet: 10:30 22/04/2019\n");
	printf("<Guilherme> Ola mundo!\n");
	printf("<Joao> Estou aqui\n");
	//gotoxy(0, irc_get_console_height()-2);

	char a;
	printf("Text so far:");
	a = getch();
	printf("%c", a);
	a = getch();
	printf("%c", a);
	printf("\n");

	return 0;
}