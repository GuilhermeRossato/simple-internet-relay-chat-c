#include <conio.h>

// waits for the user to press a single character
char irc_wait_char();

// default goto on console
void gotoxy(int x, int y);

// default clear console screen
void clrscr();

#ifdef _WIN32
#include <windows.h>

void gotoxy(int x, int y) {
  COORD coord;
  coord.X = x;
  coord.Y = y;
  SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}
void clrscr() {
	system("cls");
}
#endif

char irc_wait_char() {
	return getch();
}

int irc_get_console_height() {
	#ifdef _WIN32
		CONSOLE_SCREEN_BUFFER_INFO sbInfo;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbInfo);
		//printf("dwMaximumWindowSize %d %d\n", sbInfo.dwMaximumWindowSize.X, sbInfo.dwMaximumWindowSize.Y);
		//printf("SMALL_RECT %d %d %d %d\n", sbInfo.srWindow.Top, sbInfo.srWindow.Right, sbInfo.srWindow.Bottom, sbInfo.srWindow.Left);
		//return sbInfo.dwSize.Y;
		return sbInfo.srWindow.Bottom;
	#else
		struct winsize w;
		ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
		return w.ws_row;
	#endif
}