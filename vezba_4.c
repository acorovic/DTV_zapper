#include <stdio.h>
#include <directfb.h>
#include <stdint.h>
#include <linux/input.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include "stream_controller.h"

#define NUM_EVENTS 5
#define NON_STOP 1

/* error codes */
#define NO_ERROR 0
#define ERROR 1

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

static int32_t inputFileDesc;

int32_t getKeys(int32_t count, uint8_t* buf, int32_t* eventRead);

void writeEventKeycode(int32_t keycode);

void clrScr(int signo); 

void* fadeDraw(void* keycode);

void fade(int32_t keycode);

static IDirectFBSurface *primary = NULL;
IDirectFB *dfbInterface = NULL;
static int screenWidth = 0;
static int screenHeight = 0;

timer_t timerId;
struct itimerspec timerSpec;
struct itimerspec timerSpecOld;
int32_t timerFlags = 0;
struct sigevent signalEvent;

int32_t main(int32_t argc, char** argv)
{
	DFBSurfaceDescription surfaceDesc;

	const char* dev = "/dev/input/event0";
	char deviceName[20];
	struct input_event* eventBuf;
	uint32_t eventCnt;
	uint32_t i;

	uint8_t exitFlag = 0;
/* Initialization */
	tuner_init(754000000);

	player_init();


/* Deinitialization */
	player_deinit();

	tuner_deinit();

	signalEvent.sigev_notify = SIGEV_THREAD;
	signalEvent.sigev_notify_function = (void*)clrScr;
	signalEvent.sigev_value.sival_ptr = NULL;
	signalEvent.sigev_notify_attributes = NULL;
	timer_create(CLOCK_REALTIME, &signalEvent, &timerId);

	memset(&timerSpec, 0, sizeof(timerSpec));

	timerSpec.it_value.tv_sec = 3;
	timerSpec.it_value.tv_nsec = 0;

	/* Initialize remote */
	inputFileDesc = open(dev, O_RDWR);
	if(inputFileDesc == -1) {
		printf("error while opening device for remote \n");
		return ERROR;
	}

	ioctl(inputFileDesc, EVIOCGNAME(sizeof(deviceName)), deviceName);
	printf("RC device opened succesfully %s \n", deviceName);

	eventBuf = malloc(NUM_EVENTS * sizeof(struct input_event));
	if(!eventBuf) {
		printf("error allocating memory\n");
		return ERROR;
	}

	/* initialize DirectFB */
	DFBCHECK(DirectFBInit(NULL, NULL));
    /* fetch the DirectFB interface */
	DFBCHECK(DirectFBCreate(&dfbInterface));
    /* tell the DirectFB to take the full screen for this application */
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));
	
    
    /* create primary surface with double buffering enabled */
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK (dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));
    
    
    /* fetch the screen size */
    DFBCHECK (primary->GetSize(primary, &screenWidth, &screenHeight));
    
    
    /* clear the screen before drawing anything (draw black full screen rectangle)*/
    
    DFBCHECK(primary->SetColor(/*surface to draw on*/ primary,
                               /*red*/ 0x00,
                               /*green*/ 0x00,
                               /*blue*/ 0x00,
                               /*alpha*/ 0xff));
	DFBCHECK(primary->FillRectangle(/*surface to draw on*/ primary,
                                    /*upper left x coordinate*/ 0,
                                    /*upper left y coordinate*/ 0,
                                    /*rectangle width*/ screenWidth,
                                    /*rectangle height*/ screenHeight));
	
	DFBCHECK(primary->Flip(primary,
                           /*region to be updated, NULL for the whole surface*/NULL,
                           /*flip flags*/0));

	while(NON_STOP) {
		if(getKeys(NUM_EVENTS, (uint8_t*)eventBuf, &eventCnt)) {
			printf("error while reading input events! \n");
			return ERROR;
		}

		for(i = 0; i < eventCnt; i++) {
			if(eventBuf[i].value == 1 && eventBuf[i].code != 102) {	
				//writeEventKeycode(eventBuf[i].code);
				fade(eventBuf[i].code);
			}

			if(eventBuf[i].code == 102) {
				exitFlag = 1;
			}			
		}

		if(exitFlag == 1) {
			break;
		}
	}
    
    timer_settime(timerId, 0, &timerSpec, &timerSpecOld);
	timer_delete(timerId);
        /* wait 5 seconds before terminating*/
	//sleep(5);
    
    /*clean up*/
    
	primary->Release(primary);
	dfbInterface->Release(dfbInterface);
    
    //TODO 2: add fade-in and fade-out effects
    //TODO 3: add animation - the display should start at the left part of the screen and at the right
    //TODO 4: add support for displaying up to 3 keycodes at the same time, each in a separate row

    return 0;
}

void fade(int32_t keycode) {
	pthread_t thread;

	timer_settime(timerId, timerFlags, &timerSpec, &timerSpecOld);
	pthread_create(&thread, NULL, fadeDraw, &keycode);
	pthread_join(thread, NULL);	
}

void* fadeDraw(void* keycode) {
	char keycode_str[5];
	int16_t i = 0;
	uint32_t keycode_param;

	keycode_param = *((int32_t*)keycode);

	printf("keycode %d \n", keycode_param); 
	/* draw text */
	IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	
	/* specify the height of the font by raising the appropriate flag and setting the height value */
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = 48;	

	/* create the font and set the created font for primary surface text drawing */
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
  
	sprintf(keycode_str, "%d", keycode_param);

	DFBCHECK(primary->SetColor(/*surface to draw on*/ primary,
								   /*red*/ 0x00,
								   /*green*/ 0x00,
								   /*blue*/ 0x00,
								   /*alpha*/ 0xff));
	DFBCHECK(primary->FillRectangle(/*surface to draw on*/ primary,
										/*upper left x coordinate*/ 0,
										/*upper left y coordinate*/ 0,
										/*rectangle width*/ screenWidth,
										/*rectangle height*/ screenHeight));

	DFBCHECK(primary->Flip(primary,
							   /*region to be updated, NULL for the whole surface*/NULL,
							   /*flip flags*/0));
	

	for(i = 0; i < 128; i+=5) {
		DFBCHECK(primary->SetColor(primary, 0x03, 0x03, 0xff, i));
		DFBCHECK(primary->FillRectangle(primary, screenWidth/5, screenHeight/5, screenWidth/3, screenHeight/3));
		
		DFBCHECK(primary->SetColor(primary, 0x00, 0xff, 0x00, i));
		DFBCHECK(primary->FillRectangle(primary, screenWidth/5 + 10, screenHeight/5 + 10, screenWidth/3 - 20, screenHeight/3 - 20));
		
		
		DFBCHECK(primary->SetColor(primary, 0xFF, 0x00, 0x00, i));

			/* draw the text */
		DFBCHECK(primary->DrawString(primary,
									 /*text to be drawn*/ keycode_str,
									 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
									 /*x coordinate of the lower left corner of the resulting text*/ (screenWidth/3 + screenWidth/5)/2,
									 /*y coordinate of the lower left corner of the resulting text*/ (screenHeight/3 + screenHeight/5)/2,
									 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
		
		DFBCHECK(primary->Flip(primary,
							   /*region to be updated, NULL for the whole surface*/NULL,
							   /*flip flags*/0));
	}

	
	for(i = 125; i >= 0; i-=5) {
		DFBCHECK(primary->SetColor(primary, 0x03, 0x03, 0xff, i));
		DFBCHECK(primary->FillRectangle(primary, screenWidth/5, screenHeight/5, screenWidth/3, screenHeight/3));
		
		DFBCHECK(primary->SetColor(primary, 0x00, 0xff, 0x00, i));
		DFBCHECK(primary->FillRectangle(primary, screenWidth/5 + 10, screenHeight/5 + 10, screenWidth/3 - 20, screenHeight/3 - 20));
		
		
		DFBCHECK(primary->SetColor(primary, 0xFF, 0x00, 0x00, i));

			/* draw the text */
		DFBCHECK(primary->DrawString(primary,
									 /*text to be drawn*/ keycode_str,
									 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
									 /*x coordinate of the lower left corner of the resulting text*/ (screenWidth/3 + screenWidth/5)/2,
									 /*y coordinate of the lower left corner of the resulting text*/ (screenHeight/3 + screenHeight/5)/2,
									 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
		
		DFBCHECK(primary->Flip(primary,
							   /*region to be updated, NULL for the whole surface*/NULL,
							   /*flip flags*/0));
	}
}

void writeEventKeycode(int32_t keycode) {
	/* rectangle drawing */
	char keycode_str[5];   

	timer_settime(timerId, timerFlags, &timerSpec, &timerSpecOld);

	DFBCHECK(primary->SetColor(/*surface to draw on*/ primary,
                               /*red*/ 0x00,
                               /*green*/ 0x00,
                               /*blue*/ 0x00,
                               /*alpha*/ 0xff));
	DFBCHECK(primary->FillRectangle(/*surface to draw on*/ primary,
                                    /*upper left x coordinate*/ 0,
                                    /*upper left y coordinate*/ 0,
                                    /*rectangle width*/ screenWidth,
                                    /*rectangle height*/ screenHeight));

    DFBCHECK(primary->SetColor(primary, 0x03, 0x03, 0xff, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/5, screenHeight/5, screenWidth/3, screenHeight/3));
    
	DFBCHECK(primary->SetColor(primary, 0x00, 0xff, 0x00, 0xff));
    DFBCHECK(primary->FillRectangle(primary, screenWidth/5 + 10, screenHeight/5 + 10, screenWidth/3 - 20, screenHeight/3 - 20));
    
	/* draw text */
	IDirectFBFont *fontInterface = NULL;
	DFBFontDescription fontDesc;
	
    /* specify the height of the font by raising the appropriate flag and setting the height value */
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = 48;
	
	DFBCHECK(primary->SetColor(primary, 0xFF, 0x00, 0x00, 0xff));

    /* create the font and set the created font for primary surface text drawing */
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, "/home/galois/fonts/DejaVuSans.ttf", &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
   
	sprintf(keycode_str, "%d", keycode);

    /* draw the text */
	DFBCHECK(primary->DrawString(primary,
                                 /*text to be drawn*/ keycode_str,
                                 /*number of bytes in the string, -1 for NULL terminated strings*/ -1,
                                 /*x coordinate of the lower left corner of the resulting text*/ (screenWidth/3 + screenWidth/5)/2,
                                 /*y coordinate of the lower left corner of the resulting text*/ (screenHeight/3 + screenHeight/5)/2,
                                 /*in case of multiple lines, allign text to left*/ DSTF_LEFT));
	
	DFBCHECK(primary->Flip(primary,
                           /*region to be updated, NULL for the whole surface*/NULL,
                           /*flip flags*/0));
}

void clrScr(int signo) {
	DFBCHECK(primary->SetColor(/*surface to draw on*/ primary,
                               /*red*/ 0x00,
                               /*green*/ 0x00,
                               /*blue*/ 0x00,
                               /*alpha*/ 0xff));
	DFBCHECK(primary->FillRectangle(/*surface to draw on*/ primary,
                                    /*upper left x coordinate*/ 0,
                                    /*upper left y coordinate*/ 0,
                                    /*rectangle width*/ screenWidth,
                                    /*rectangle height*/ screenHeight));
	
	DFBCHECK(primary->Flip(primary,
                           /*region to be updated, NULL for the whole surface*/NULL,
                           /*flip flags*/0));
}

int32_t getKeys(int32_t count, uint8_t* buf, int32_t* eventsRead) 
{
	int32_t ret = 0;
	
	ret = read(inputFileDesc, buf, (size_t)(count * (int)sizeof(struct input_event)));
	if(ret <= 0) {
		printf("error code %d \n", ret);
		return ERROR;
	}

	*eventsRead = ret/(int)sizeof(struct input_event);

	return NO_ERROR;
}
