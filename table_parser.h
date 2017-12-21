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

#define LOCK_TIME               10            /**< Max waiting time to PAT and PMT */
#define LOCK_TIME_TDT_TOT       30            /**< Max waiting time to TDT and TOT */
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

/**
 * @brief Struct wich contain all PMT info about channel
 */
typedef struct pmt {
	uint16_t has_video;         /**< Video or audio channel */
	uint16_t video_pid;         /**< Video PID */
	uint16_t audio_pid[4];      /**< Audio PIDs */
	uint16_t has_teletext;      /**< If channel has teltext */
} pmt_t;

/**
 * @brief Info about channel which will be played
 */
typedef struct channel_info {
    int8_t channel_no;
    int32_t video_pid;
    int32_t audio_pid;
    int8_t has_teletext;
    int8_t has_video;
} channel_t;

/**
 * @brief PAT info
 */
typedef struct service {
    uint16_t program_no;        /**< Program number on STB */
    uint16_t pid;               /**< Program PID for PMT parsing */
} service_t;

/**
 * @brief Time info from TDT and TOT
 */
typedef struct time{
	int8_t hour;                /**< Hour */
	int8_t minute;              /**< Minute */
	int8_t tdt_completed;       /**< TDT completed flag */
	int8_t tot_completed;       /**< TOT completed flag */
} tdt_time_t;

/**
 * @brief Function which filters PAT
 *
 * Function sets PAT filter demux and waits max LOCK_TIME for
 * PAT to arrive. Blocking function because of LOCK_TIME waiting
 * interval
 *
 * @param pat_info Pointer to service array where filtered PAT
 * info will be stored
 *
 * @return ERR or NO_ERR
 */
int8_t filter_pat(service_t* pat_info);

/**
 * @brief Function which filters PMT
 *
 * Function sets PMT filter on demux and waits max LOCK_TIME for
 * PMT to arrive. Blocking function because of LOCK_TIME waiting
 * interval
 *
 * @param channel_pid PID of channel whose PMT will be filtered
 * @param channel_pmt_info Pointer to struct where filtered info
 * will be stored
 *
 * @return ERR or NO_ERR
 */
int8_t filter_pmt(uint16_t channel_pid, pmt_t* channel_pmt_info);

/**
 * @brief Function which filters TDT
 *
 * Function sets TDT filter on demux and waits max LOCK_TIME_TDT_TOT
 * for TDT to arrive. Blocking function because of LOCK_TIME_TDT_TOT
 * waiting interval.
 *
 * @param player_time Pointer to struct where filtered time will be
 * stored
 *
 * @return ERR or NO_ERR
 */
int8_t filter_tdt(tdt_time_t* player_time);

/**
 * @brief Function which filters TOT
 *
 * Function sets TOT filter on demux and waits max LOCK_TIME_TDT_TOT
 * for TDT to arrive. Blocking function because of LOCK_TIME_TDT_TOT
 * waiting interval.
 *
 * @param player_time Pointer to struct where filtered time will be
 * stored
 *
 * @return ERR or NO_ERR
 */
int8_t filter_tot(tdt_time_t* player_time);

/**
 * @brief Function which returns whether TDT filtering is complete
 *
 * @return 0 not complete, 1 completed
 */
int8_t parser_get_time_completed();

/**
 * @brief Function which returns whether TOT filtering is complete
 *
 * @return 0 not completed, 1 completed
 */
int8_t parser_get_timezone_completed();

/**
 * @brief Function sets TDT filter to demux
 */
void start_tdt_parsing();

/**
 * @brief Function removes TDT filter from demux
 */
void stop_tdt_parsing();

/**
 * @brief Function sets TOT filter to demux
 */
void start_tot_parsing();

/**
 * @brief Function removes TOT filter from demux
 */
void stop_tot_parsing();

/**
 * @brief Function returns time struct from TDT and TOT
 *
 * @return TDT and TOT time struct
 */
tdt_time_t parser_get_time();

#endif
