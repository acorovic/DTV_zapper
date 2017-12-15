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

#define NUM_EVENTS 5
#define MAX_CHANNEL 10
#define MIN_CHANNEL 0
#define MIN_VOL_LEVEL 0
#define MAX_VOL_LEVEL 10

int8_t remote_init();

void remote_set_decode_keypress(void (*callback)(uint16_t keycode));

int8_t remote_deinit();

#endif
