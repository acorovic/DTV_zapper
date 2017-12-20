#ifndef TABLE_PARSER_H
#define TABLE_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "tdp_api.h"

#define LOCK_TIME 10

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

typedef struct pmt {
	uint16_t has_video;
	uint16_t video_pid;
	uint16_t audio_pid[4];
	uint16_t has_teletext;
} pmt_t;

typedef struct channel_info {
    int8_t channel_no;
    int32_t video_pid;
    int32_t audio_pid;
    int8_t has_teletext;
    int8_t has_video;
} channel_t;

typedef struct service {
    uint16_t program_no;
    uint16_t pid;
} service_t;

typedef struct time{
	int8_t hour;
	int8_t minute;
} tdt_time_t;

int8_t filter_pat(service_t* pat_info);
int8_t filter_pmt(uint16_t channel_pid, pmt_t* channel_pmt_info);
int8_t filter_tdt(tdt_time_t* player_time);
int8_t filter_tot(tdt_time_t* player_time);

#endif
