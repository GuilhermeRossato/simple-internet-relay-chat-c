#define IRC_TIMER_PERIOD_MILLISECONDS	2000

int irc_is_timer_active = 0;

#ifdef _WIN32
#include <windows.h>

HANDLE timer_handle;
void * _irc_callback_func;

void _irc_internal_time_called(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
{
	DeleteTimerQueueTimer(NULL, timer_handle, NULL);
	timer_handle = NULL;
	if (irc_is_timer_active == 1) {
		CreateTimerQueueTimer(&timer_handle, NULL, _irc_internal_time_called, NULL, 1000, 0, WT_EXECUTEDEFAULT);
	}
	void (* func)();
	func = _irc_callback_func;
	func();
}

int irc_start_timer(void * callback) {
	if (irc_is_timer_active) {
		return 0;
	}
	irc_is_timer_active = 1;
	_irc_callback_func = callback;
	CreateTimerQueueTimer(&timer_handle, NULL, _irc_internal_time_called, NULL, 1000, 0, WT_EXECUTEDEFAULT);
	return 1;
}

int irc_stop_timer() {
	irc_is_timer_active = 0;
}

#else

#include <signal.h>

void _irc_internal_time_called(int sig) {
	if (sig == SIGFPE) {

	}
	printf("got signal %d and %d\n", sig);
	printf("\n4 second time-out reached ... aborting !\n");
	exit(0);
}

void _irc_internal_time_called(PVOID lpParameter, BOOLEAN TimerOrWaitFired) {
	void (* func)();
	func = _irc_callback_func;
	func();
}

int irc_start_timer(void * callback) {
	signal(SIGFPE, alarm_func);
	int time_seconds = (IRC_TIMER_PERIOD_MILLISECONDS/1000 <= 0)?1:(IRC_TIMER_PERIOD_MILLISECONDS/1000);
	alarm(time_seconds);
}

int irc_stop_timer() {
	irc_is_timer_active = 0;
}

#endif
