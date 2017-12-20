#ifndef COMMON_H
#define COMMON_H

/* input event values for 'EV_KEY' type */
#define EV_VALUE_RELEASE    0
#define EV_VALUE_KEYPRESS   1
#define EV_VALUE_AUTOREPEAT 2

/* input event codes */
#define KEYCODE_P_MINUS 	61
#define KEYCODE_P_PLUS 		62
#define KEYCODE_EXIT 		102
#define KEYCODE_INFO 		358
#define KEYCODE_VOL_PLUS    63
#define KEYCODE_VOL_MINUS   64
#define KEYCODE_VOL_MUTE    60
#define KEYCODE_P_1			2
#define KEYCODE_P_2			3
#define KEYCODE_P_3			4
#define KEYCODE_P_4			5
#define KEYCODE_P_5			6
#define KEYCODE_P_6			7
#define KEYCODE_P_0			11
/* APP constraints */
#define MAX_CHANNEL     6
#define MIN_CHANNEL     0
#define MIN_VOL_LEVEL   0
#define MAX_VOL_LEVEL   10

/* error codes */
#define NO_ERR 		0
#define ERR			1
#define PTHREAD_ERROR 	2

/* table infos */
#define PAT_PID     0
#define PAT_TABLEID 0
#define PMT_TABLEID 2
#define TDT_PID 	0x0014
#define TDT_TABLEID 0x70
#define TOT_PID		0x0014
#define TOT_TABLEID	0x73
#define TOT_DESC	0x58



#endif /* COMMON_H */
