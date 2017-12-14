#ifndef GRAPHIC_CONTROLLER_H
#define GRAPHIC_CONTROLLER_H

#include <stdio.h>
#include <directfb.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>
#include "common.h"
#include <signal.h>
#include "timer.h"

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

#define CHANNEL_BANNER_WIDTH 	100
#define CHANNEL_BANNER_HEIGHT 	70
#define TIME_BANNER_WIDTH 		500
#define TIME_BANNER_HEIGHT 		100
#define FONT_HEIGHT 			40


int8_t graphic_init();
int8_t graphic_deinit();

int8_t graphic_draw_channel_no(uint8_t channel_no);
int8_t graphic_draw_time(tdt_time_t time);

#endif
