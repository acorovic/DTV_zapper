#ifndef TIMER_H
#define TIMER_H

#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct custom_timer{
	timer_t timer_id;               /**< timer_id used for creation and starting timer */
	struct itimerspec timerSpec;    /**< timerSpec used for setting timer interval */
	struct itimerspec timerSpecOld; /**< timerSpecOld used for setting timer interval */
	struct sigevent event;          /**< signal which is called when signal expire */
	void (*callback)();             /**< callback which is called when interval expire */
} custom_timer_t;

/**
 * @brief Function creates timer and set up signals and callback
 *
 * @param timer
 * @param callbck which is called when timer interval expire
 */
void custom_timer_create(custom_timer_t* timer, void (*callbck)());

/**
 * @brief Function which starts interval countdown
 *
 * @param timer Timer which will count
 * @param seconds Time interval in seconds
 */
void custom_timer_start(custom_timer_t* timer, int8_t seconds);

/**
 * @brief Fucntion which deletes timer
 *
 * @param timer Timer to be deleted
 */
void custom_timer_delete(custom_timer_t* timer);

#endif
