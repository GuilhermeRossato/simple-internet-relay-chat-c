// waits for the user to press a single character
char irc_wait_char();

// default goto on console
void gotoxy(int x, int y);

// default clear console screen
void clrscr();

#ifdef _WIN32

#include <conio.h>
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

#else

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>

void gotoxy(int x,int y) {
	printf("%c[%d;%df",0x1B, y, x);
}
/*
int getch(void) {
	int c=0;

	struct termios org_opts, new_opts;
	int res=0;
	//-----  store old settings -----------
	res = tcgetattr(STDIN_FILENO, &org_opts);

	//---- set new terminal parms --------
	memcpy(&new_opts, &org_opts, sizeof(new_opts));
	new_opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
	c = getchar();

	res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);

	return c;
}
*/
static struct termios old, new;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old); /* grab old terminal i/o settings */
  new = old; /* make new settings same as old settings */
  new.c_lflag &= ~ICANON; /* disable buffered i/o */
  if (echo) {
      new.c_lflag |= ECHO; /* set echo mode */
  } else {
      new.c_lflag &= ~ECHO; /* set no echo mode */
  }
  tcsetattr(0, TCSANOW, &new); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios(void) 
{
  tcsetattr(0, TCSANOW, &old);
}

/* Read 1 character - echo defines echo mode */
char getch_(int echo) 
{
  char ch;
  initTermios(echo);
  ch = getchar();
  resetTermios();
  return ch;
}

/* Read 1 character without echo */
char getch(void) 
{
  return getch_(0);
}


void clrscr() {
	printf("\033[H\033[J");
	//system("clear");
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
