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

/* error codes */
#define NO_ERROR 		0
#define ERROR			1
#define PTHREAD_ERROR 	2

/* table infos */
#define PAT_PID     0
#define PAT_TABLEID 0
#define PMT_TABLEID 2
#endif /* COMMON_H */
