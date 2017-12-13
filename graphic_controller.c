#include "graphic_controller.h"

static IDirectFBSurface *primary = NULL;
static IDirectFB *dfbInterface = NULL;
static DFBSurfaceDescription surfaceDesc;
static IDirectFBFont *fontInterface = NULL;
static DFBFontDescription fontDesc;
static char font_path[] = "/home/galois/fonts/DejaVuSans.ttf";
static int32_t screen_width;
static int32_t screen_height;

static pthread_t render_thread;
static int8_t render_running;

static int8_t draw_channel_no_flag;
static char channel_no_str[5];
static timer_t timer_channel;
static int32_t timer_flags;
static struct itimerspec timerSpec;
static struct itimerspec timerSpecOld;

static struct sigevent channel_no_event;

static void draw_channel_no_fcn() {
	DFBCHECK(primary->SetColor(primary, 0x00, 0xff, 0x00, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, CHANNEL_BANNER_WIDTH, CHANNEL_BANNER_HEIGHT));

	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0xff, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 10, 10, CHANNEL_BANNER_WIDTH - 20,
													 CHANNEL_BANNER_HEIGHT - 20));
	DFBCHECK(primary->SetColor(primary, 0xff, 0x00, 0x00, 0xff));
	DFBCHECK(primary->DrawString(primary, channel_no_str, -1, CHANNEL_BANNER_WIDTH/2,
								CHANNEL_BANNER_HEIGHT/2, DSTF_LEFT));
}

static void* render_fcn() {
	while(render_running) {
		/* Clear graphic */
		DFBCHECK(primary->SetColor(primary, 0x00, 0xff, 0x00, 0x00));
		DFBCHECK(primary->FillRectangle(primary, 0, 0, CHANNEL_BANNER_WIDTH, CHANNEL_BANNER_HEIGHT));
		/* Check channel no flag */
		if(draw_channel_no_flag) {
			draw_channel_no_fcn();
		}

		DFBCHECK(primary->Flip(primary, NULL, 0));
	}
}

int8_t graphic_draw_channel_no(uint8_t channel_no) {
	memset(&timerSpec, 0, sizeof(timerSpec));
	timerSpec.it_value.tv_sec = 3;
	timerSpec.it_value.tv_nsec = 0;
	timer_settime(timer_channel, timer_flags, &timerSpec, &timerSpecOld);

	draw_channel_no_flag = 1;
}

static void clr_channel_no_flag() {
	draw_channel_no_flag = 0;
}

int8_t graphic_init() {
	/* Initialize directFB */
	DFBCHECK(DirectFBInit(NULL, NULL));
	/* Fetch the directFB interface */
	DFBCHECK(DirectFBCreate(&dfbInterface));
	/* Tell the directFB to take the full screen for this application */
	DFBCHECK(dfbInterface->SetCooperativeLevel(dfbInterface, DFSCL_FULLSCREEN));

	/* Create primary surface with double buffering enabled */
	surfaceDesc.flags = DSDESC_CAPS;
	surfaceDesc.caps = DSCAPS_PRIMARY | DSCAPS_FLIPPING;
	DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &primary));

	/* Fetch the screen size */
	DFBCHECK(primary->GetSize(primary, &screen_width, &screen_height));

	/* Clear the screen before drawing anything (draw black full screen rect(alpha 0)) */
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));
	DFBCHECK(primary->Flip(primary, NULL, 0));

	/* Specify the font height */
	fontDesc.flags = DFDESC_HEIGHT;
	fontDesc.height = FONT_HEIGHT;

	DFBCHECK(dfbInterface->CreateFont(dfbInterface, font_path, &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));
	
	channel_no_event.sigev_notify = SIGEV_THREAD;
	channel_no_event.sigev_notify_function = (void*)clr_channel_no_flag;
	channel_no_event.sigev_value.sival_ptr = NULL;
	channel_no_event.sigev_notify_attributes = NULL;
	timer_create(CLOCK_REALTIME, &channel_no_event, &timer_channel);

	render_running = 1;
	if (pthread_create(&render_thread, NULL, &render_fcn, NULL))
    {
        render_running = 0;
        printf("Error creating thread for remote\n");
        return ERR;
    }

	return NO_ERR;
}

int8_t graphic_deinint() {
	render_running = 0;

	if (pthread_join(render_thread, NULL))
    {
        printf("Error during render_thread join!\n");
        return ERR;
    }

	primary->Release(primary);
	dfbInterface->Release(dfbInterface);

	return NO_ERR;
}
