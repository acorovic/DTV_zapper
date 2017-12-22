#ifndef GRAPHIC_CONTROLLER_H
#define GRAPHIC_CONTROLLER_H

#include <stdio.h>
#include <directfb.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include "common.h"
#include "timer.h"
#include "stream_controller.h"

/**
 * @brief Macro function used to check errors from DirectFb
 *
 * @param x...
 *
 * @return
 */
#define DFBCHECK(x...)                                      \
{                                                           \
DFBResult err = x;                                          \
                                                            \
if (err != DFB_OK)                                          \
  {                                                         \
    fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );  \
    DirectFBErrorFatal( #x, err );                          \
  }                                                         \
}

#define CHANNEL_BANNER_WIDTH 	520
#define CHANNEL_BANNER_HEIGHT 	220
#define TIME_BANNER_WIDTH 		500
#define TIME_BANNER_HEIGHT 		100
#define FONT_HEIGHT 			40
#define VOLUME_IMAGE_NO        	11  /**< Number of volume images to be loaded */
#define CHANNEL_INFO_TEXT_W		720
#define CHANNEL_INFO_TEXT_H		870
#define INFO_INTERVAL_S         3   /**< Time interval in s after which info banner will be hidden */
#define TIME_INTERVAL_S         3   /**< Time interval in s after which time banner will be hidden */

/**
 * @brief Function which initializes DirectFB and spawns render thread.
 *
 * Function fetches screen width and height, creates surface for drawing
 * graphical elements, loads images used for drawing volume states etc.
 * Function also creates render thread and timers used for synchronization
 * of all elements which need to be drawn.
 *
 * @return NO_ERR or ERR
 */
int8_t graphic_init();

/**
 * @brief Function which properly ends working with DirectFB.
 *
 * Function properly ends render thread and release surfaces used for
 * drawing elements. It also deletes timers used for synchronization
 *
 * @return NO_ERR or ERR
 */
int8_t graphic_deinit();

/**
 * @brief Function which set up flags used by render loop for drawing channel info
 *
 * Function checks if channel which is sent as parameter has audio, video
 * and teletext and sets flags for render loop accordingly. Also, starts timer
 * which will hide those infos from screen after INFO_INTERVAL_S seconds.
 *
 * @param channel whose info will be drawn
 */
void graphic_draw_channel_info(channel_t channel);

/**
 * @brief Function which set up flags used by render loop for drawing time info
 *
 * @param time current time on STB
 */
void graphic_draw_time(tdt_time_t time);

/**
 * @brief Function which set up flags used by render loop for drawing images which
 * represent volume levels
 *
 * @param volume_level current volume level on STB
 */
void graphic_draw_volume_level(int8_t volume_level);

#endif
