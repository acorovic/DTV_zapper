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
#include "table_parser.h"

/**
 * @brief Function initializes tuner and locks it to frequecy
 *
 * If lock is successful, function inits player and opens source.
 * Function is blocking, beacause it waits for LOCK_TIME for tuner
 * to lock on frequency
 *
 * @param frequency Frequency in Hz
 * @param bandwidth
 * @param modulation
 *
 * @return ERR or NO_ERR
 */
int8_t tuner_init(uint32_t frequency, int32_t bandwidth, enum t_Module modulation);

int8_t tuner_deinit();

int8_t demux_init(uint32_t PID, uint32_t tableID, int32_t(*demux_filter_callback)(uint8_t* buffer));

int8_t demux_deinit(int32_t(*demux_filter_callback)(uint8_t* buffer));

int8_t player_play_channel(channel_t* channel, enum t_StreamType video_type, enum t_StreamType audio_type);

int8_t player_play_init_channel(channel_t* channel, enum t_StreamType video_type, enum t_StreamType audio_type);
tdt_time_t player_get_time();

int8_t player_stop_channel();

int8_t player_set_volume(uint8_t vol_level);

#endif
