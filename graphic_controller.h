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

/* helper macro for error checking */
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
#define VOLUME_IMAGE_NO        	11 
#define CHANNEL_INFO_TEXT_W		20
#define CHANNEL_INFO_TEXT_H		60

int8_t graphic_init();
int8_t graphic_deinit();

int8_t graphic_draw_channel_info(channel_t channel);
int8_t graphic_draw_time(tdt_time_t time);
int8_t graphic_draw_volume_level(int8_t volume_level);
int8_t graphic_draw_volume_mute();

#endif
