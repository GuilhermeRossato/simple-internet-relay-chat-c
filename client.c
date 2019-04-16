#include "./src/irc.h"

int main() {
	irc_send("00:00:00:00:00", "hello world");
	irc_send("00:00:00:00:00", "/create #channel");
	irc_send("00:00:00:00:00", "/quit #channel");
	irc_send("00:00:00:00:00", "/exit");
	return 0;
}