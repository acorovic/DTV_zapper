#ifndef STREAM_CONTROLLER_H
#define STREAM_CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "errno.h"
#include "tdp_api.h"

#define LOCK_TIME 5

static inline void textColor(int32_t attr, int32_t fg, int32_t bg)
{
   char command[13];

   /* Command is the control command to the terminal */
   sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
   printf("%s", command);
}

#define ASSERT_TDP_RESULT(x,y)  if(NO_ERROR == x) \
                                    printf("%s success\n", y); \
                                else{ \
                                    textColor(1,1,0); \
                                    printf("%s fail\n", y); \
                                    textColor(0,7,0); \
                                    return -1; \
                                }

static int32_t tuner_callback(t_LockStatus status);

int8_t tuner_init(uint32_t frequency);
int8_t tuner_deinit();

int8_t player_init();
int8_t player_deinit();

int8_t demux_init(uint32_t PID, uint32_t tableID, int32_t(*demux_filter_callback)(uint8_t* buffer));

int8_t tuner_deinit();


#endif
