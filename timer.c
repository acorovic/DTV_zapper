#include "timer.h"

void custom_timer_create(custom_timer_t* timer, void (*callbck)())
{
	timer->event.sigev_notify = SIGEV_THREAD;
	timer->event.sigev_notify_function = (void*)callbck;
	timer->event.sigev_value.sival_ptr = NULL;
	timer->event.sigev_notify_attributes = NULL;

	timer_create(CLOCK_REALTIME, &(timer->event), &(timer->timer_id));
	/* Add guards */
}

void custom_timer_start(custom_timer_t* timer, int8_t seconds)
{
	memset(&(timer->timerSpec), 0, sizeof(timer->timerSpec));

	timer->timerSpec.it_value.tv_sec = 3;
	timer->timerSpec.it_value.tv_nsec = 0;
	timer_settime(timer->timer_id, 0, &(timer->timerSpec), &(timer->timerSpecOld));
}

void custom_timer_delete(custom_timer_t* timer)
{
	timer_delete(timer->timer_id);
}
