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

/**
 * @brief Function deinitializes tuner and closes players
 *
 *
 * @return ERR or NO_ERR
 */
int8_t tuner_deinit();

/**
 * @brief Function sets demux filter and registers callback
 *
 * @param PID to be filtered
 * @param tableID to be filtered
 * @param demux_filter_callback callback to be called when filtering complete
 *
 * @return ERR or NO_ERR
 */
int8_t demux_init(uint32_t PID, uint32_t tableID, int32_t(*demux_filter_callback)(uint8_t* buffer));

/**
 * @brief Function frees filter and removes callback
 *
 * @param demux_filter_callback to be unregistered
 *
 * @return ERR or NO_ERR
 */
int8_t demux_deinit(int32_t(*demux_filter_callback)(uint8_t* buffer));

/**
 * @brief Function plays channel by creating its audio and/or video stream
 *
 * Function stops audio and/or video stream from previous channel. It also 
 * checks if TDT or TOT filtering is running and stops it to do PMT filtering
 * to get the pmt_info of the current channel. After doing PMT filtering
 * function continues doing TDT or TOT filtering if necessary.
 *
 * @param channel to be played
 * @param video_type of the stream to be played
 * @param audio_type of the stream to be played
 *
 * @return ERR or NO_ERR
 */
int8_t player_play_channel(channel_t* channel, enum t_StreamType video_type, enum t_StreamType audio_type);

/**
 * @brief Function plays initial channel read from init file by creating its
 * audio and/or video stream.
 *
 * Function first does PAT filtering, Than it checks if audio or video PID passed
 * as arugment are the same as from the PMT table. It also starts TDT filtering
 * in the background.
 *
 * @param channel to be played
 * @param video_type of the stream to be played
 * @param audio_type of the stream to be played
 *
 * @return ERR or NO_ERR
 */
int8_t player_play_init_channel(channel_t* channel, enum t_StreamType video_type, enum t_StreamType audio_type);

/**
 * @brief Function sets volume level to the player
 *
 * @param vol_level to be set to player
 *
 * @return ERR or NO_ERR
 */
int8_t player_set_volume(uint8_t vol_level);

/**
 * @brief Function check if TDT or TOT filtering needs to be started
 * and starts them
 *
 * @return tdt_time_t struct which contains time and flags if TDT and TOT
 * filtering are completed
 */
tdt_time_t player_get_time();

#endif
