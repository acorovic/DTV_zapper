#ifndef REMOTE_CONTROLLER_H
#define REMOTE_CONTROLLER_H

#include <stdio.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include "common.h"

#define NUM_EVENTS 5        /**< Number of events which can be read from remote in one turn */

/**
 * @brief Function opens input device file and spawns thread which check keys
 *
 * @return ERR or NO_ERR
 */
int8_t remote_init();

/**
 * @brief Function set callback which is called when the key is pressed
 *
 * @param callback Pointer to callback function
 */
void remote_set_decode_keypress(void (*callback)(uint16_t keycode));

/**
 * @brief Function stops the thread which check for events on remote
 *
 * @return ERR or NO_ERR
 */
int8_t remote_deinit();

#endif
