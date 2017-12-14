#ifndef TIMER_H
#define TIMER_H

#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct custom_timer{
	timer_t timer_id;
	struct itimerspec timerSpec;
	struct itimerspec timerSpecOld;
	struct sigevent event;
	void (*callback)();
} custom_timer_t;

void custom_timer_create(custom_timer_t* timer, void (*callbck)());
void custom_timer_start(custom_timer_t* timer, int8_t seconds);
void custom_timer_delete(custom_timer_t* timer);

#endif
