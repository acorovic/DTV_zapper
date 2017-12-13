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

#define NO_ERROR 0
#define ERROR 1
#define NUM_EVENTS 5
#define MAX_CHANNEL 10
#define MIN_CHANNEL 0
int8_t remote_init();

void remote_set_decode_keypress(void (*callback)(uint16_t keycode));

int8_t remote_deinit();

#endif
