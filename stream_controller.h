#ifndef STREAM_CONTROLLER_H
#define STREAM_CONTROLLER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
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

typedef struct pmt {
	uint16_t has_video;
	uint16_t video_pid;
	uint16_t audio_pid[4];
	uint16_t has_teletext;
} pmt_t;

int8_t tuner_init(uint32_t frequency);
int8_t tuner_deinit();

int8_t demux_init(uint32_t PID, uint32_t tableID, int32_t(*demux_filter_callback)(uint8_t* buffer));
int8_t demux_deinit(int32_t(*demux_filter_callback)(uint8_t* buffer));

int8_t player_play_channel(channel_t* channel);
tdt_time_t player_get_time();
int8_t player_stop_channel();
int8_t player_set_volume(uint8_t vol_level);

int8_t filter_pat();
int8_t filter_pmt(uint16_t channel_pid);
int8_t filter_tdt();
int8_t filter_tot();

#endif
