#include "./src/irc.h"

int receive(char * message, int message_length) {
	printf("Message received: %s\n", message);
}

int main() {
	irc_server("lo", receive);
	return 0;
}