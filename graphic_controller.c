#include "graphic_controller.h"

/* Global variables used by DirectFb */
static IDirectFBSurface *primary = NULL;
static IDirectFB *dfbInterface = NULL;
static DFBSurfaceDescription surfaceDesc;
static IDirectFBFont *fontInterface = NULL;
static DFBFontDescription fontDesc;
static IDirectFBImageProvider* volume_image_provider[VOLUME_IMAGE_NO];
static IDirectFBSurface* volume_image_surface[VOLUME_IMAGE_NO];
static int32_t screen_width;
static int32_t screen_height;

/* Paths to font and image folder */
static const char font_path[] = "/home/galois/fonts/DejaVuSans.ttf";
static const char image_folder[] = "./images/";

/* Helper vars used for drawing */
static char channel_no_str[5];
static char channel_video_pid_str[30];
static char channel_audio_pid_str[30];
static int8_t channel_has_teletext;
static const char channel_teletext_y_str[] = "Teletext: AVAILABLE";
static const char channel_teletext_n_str[] = "Teletext: UNAVAILABLE";
static const char radio_str[] = "Radio playing...";
static int8_t channel_has_video;
static char time_str[10];
static int8_t volume_level;

/* Render thread and flag to end thread */
static pthread_t render_thread;
static int8_t render_running;

/* Flags checked by render loop */
static int8_t draw_channel_info_flag;
static int8_t draw_time_flag;
static int8_t draw_volume_flag;
static int8_t draw_radio_screen_flag;
static int8_t draw_boot_screen_flag;
/* Timers used to hide graphic elements */
static custom_timer_t timer_time;
static custom_timer_t timer_channel;
static custom_timer_t timer_volume;

/* Helper functions which work with DFB, draw on surface */
static void draw_channel_info_fcn();
static void draw_time_fcn();
static void draw_volume_fcn();
static void draw_radio_screen_fcn();
static void draw_boot_screen_fcn(); 

/* Callbacks which clear flags after timers expired */
static void clr_channel_info_flag();
static void clr_time_flag();
static void clr_volume_flag();

/* Helper functions */
static int8_t load_volume_images();
static void format_time_str(tdt_time_t time, char* str);

/* Function which executes in separate thread */
static void* render_fcn();

int8_t graphic_init()
{
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
    /* Create font */
	DFBCHECK(dfbInterface->CreateFont(dfbInterface, font_path, &fontDesc, &fontInterface));
	DFBCHECK(primary->SetFont(primary, fontInterface));

    if (load_volume_images() == ERR)
    {
        printf("Couldn't load volume_images \n");
        primary->Release(primary);
        dfbInterface->Release(dfbInterface);
        return ERR;
    }

	custom_timer_create(&timer_channel, clr_channel_info_flag);
	custom_timer_create(&timer_time, clr_time_flag);
    custom_timer_create(&timer_volume, clr_volume_flag);

	render_running = 1;
	if (pthread_create(&render_thread, NULL, &render_fcn, NULL))
    {
        render_running = 0;
        printf("Error creating thread for remote\n");
        return ERR;
    }
    printf("Render thread created! \n");

	return NO_ERR;
}

int8_t graphic_deinit()
{
	int8_t i;

	render_running = 0;
	if (pthread_join(render_thread, NULL))
    {
        printf("Error during render_thread join!\n");
        return ERR;
    }

	primary->Release(primary);
	dfbInterface->Release(dfbInterface);

	custom_timer_delete(&timer_time);
	custom_timer_delete(&timer_channel);

	return NO_ERR;
}

void graphic_draw_channel_info(channel_t channel)
{
	sprintf(channel_no_str, "%d", channel.channel_no);
	sprintf(channel_audio_pid_str, "Channel audio PID:%d", channel.audio_pid);
	if (channel.has_video)
	{
		draw_radio_screen_flag = 0;
		channel_has_video = 1;
		sprintf(channel_video_pid_str, "Channel video PID:%d", channel.video_pid);
	} else
	{
		draw_radio_screen_flag = 1;
		channel_has_video = 0;
	}

	if (channel.has_teletext)
	{
		channel_has_teletext = 1;
	} else
	{
		channel_has_teletext = 0;
	}
	custom_timer_start(&timer_channel, INFO_INTERVAL_S);
	draw_channel_info_flag = 1;
}

void graphic_draw_time(tdt_time_t time)
{
	//sprintf(time_str, "%d:%d", time.hour, time.minute);
	format_time_str(time, time_str);
    custom_timer_start(&timer_time, 3);
	draw_time_flag = 1;
}

void graphic_draw_volume_level(int8_t vol_level)
{
    volume_level = vol_level;

	printf("Volume LEVEL %d \n", volume_level);
	custom_timer_start(&timer_volume, 3);
    draw_volume_flag = 1;
}

void graphic_draw_boot_screen()
{
	draw_boot_screen_flag = 1;
}

void graphic_remove_boot_screen()
{
	draw_boot_screen_flag = 0;
}


static void* render_fcn()
{
	while (render_running)
    {
		/* Clear graphic */
		DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0x00));
		DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));

		if (draw_boot_screen_flag)
		{
			draw_boot_screen_fcn();
		}
	
		/* Check radio screen flag */
		if (draw_radio_screen_flag)
		{
			draw_radio_screen_fcn();
		}
		
		/* Check channel no flag */
		if (draw_channel_info_flag)
        {
			draw_channel_info_fcn();
		}

		/* Check time flag */
		if (draw_time_flag)
        {
			draw_time_fcn();
		}

        /* Check volume flag */
        if (draw_volume_flag)
        {
            draw_volume_fcn();
        }

		DFBCHECK(primary->Flip(primary, NULL, 0));
	}
}


static void draw_channel_info_fcn()
{
    /* Make banner background */
	DFBCHECK(primary->SetColor(primary, 0x38, 0x8E, 0x3C, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, CHANNEL_BANNER_WIDTH, CHANNEL_BANNER_HEIGHT));

	DFBCHECK(primary->SetColor(primary, 0x4C, 0xAF, 0x50, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 10, 10, CHANNEL_BANNER_WIDTH - 20,
													 CHANNEL_BANNER_HEIGHT - 20));
	/* Write channel no */
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	DFBCHECK(primary->DrawString(primary, channel_no_str, -1, CHANNEL_INFO_TEXT_W,
								CHANNEL_INFO_TEXT_H, DSTF_LEFT));
	/* Write teletext message */
	if (channel_has_teletext)
	{
		DFBCHECK(primary->DrawString(primary, channel_teletext_y_str, -1, CHANNEL_INFO_TEXT_W,
								CHANNEL_INFO_TEXT_H + 40, DSTF_LEFT));
	} else
	{
		DFBCHECK(primary->DrawString(primary, channel_teletext_n_str, -1, CHANNEL_INFO_TEXT_W,
								CHANNEL_INFO_TEXT_H + 40, DSTF_LEFT));
	}
	/* Write audio PID */
	DFBCHECK(primary->DrawString(primary, channel_audio_pid_str, -1, CHANNEL_INFO_TEXT_W,
								CHANNEL_INFO_TEXT_H + 80, DSTF_LEFT));

	/* Write video PID */
	if (channel_has_video)
	{
		DFBCHECK(primary->DrawString(primary, channel_video_pid_str, -1, CHANNEL_INFO_TEXT_W,
								CHANNEL_INFO_TEXT_H + 120, DSTF_LEFT));
	}
}

static void draw_time_fcn()
{
	/* Make banner background */
	DFBCHECK(primary->SetColor(primary, 0x38, 0x8E, 0x3C, 0xff));
	DFBCHECK(primary->FillRectangle(primary, screen_width/2 - TIME_BANNER_WIDTH/2,
									 screen_height - TIME_BANNER_HEIGHT,
									 TIME_BANNER_WIDTH,
									 TIME_BANNER_HEIGHT));

	DFBCHECK(primary->SetColor(primary, 0x4C, 0xAF, 0x50, 0xff));
	DFBCHECK(primary->FillRectangle(primary, screen_width/2 - TIME_BANNER_WIDTH/2 + 10,
									 screen_height - TIME_BANNER_HEIGHT + 10,
									TIME_BANNER_WIDTH - 20,
									TIME_BANNER_HEIGHT - 20));
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	/* Write time */
    DFBCHECK(primary->DrawString(primary, time_str, -1, screen_width/2 - 100,
								screen_height - TIME_BANNER_HEIGHT/2, DSTF_LEFT));
}

static void draw_volume_fcn()
{
    /* Draw volume image */
    DFBCHECK(primary->Blit(primary, volume_image_surface[volume_level], NULL, screen_width-250, 50));
}

static void draw_radio_screen_fcn()
{
	/* Black screen */
    DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));

	/* Make banner background */
	
	DFBCHECK(primary->SetColor(primary, 0x38, 0x8E, 0x3C, 0xff));
	DFBCHECK(primary->FillRectangle(primary, screen_width/2 - TIME_BANNER_WIDTH/2,
									 screen_height/2 - TIME_BANNER_HEIGHT,
									 TIME_BANNER_WIDTH,
									 TIME_BANNER_HEIGHT));

	DFBCHECK(primary->SetColor(primary, 0x4C, 0xAF, 0x50, 0xff));
	DFBCHECK(primary->FillRectangle(primary, screen_width/2 - TIME_BANNER_WIDTH/2 + 10,
									 screen_height/2 - TIME_BANNER_HEIGHT + 10,
									TIME_BANNER_WIDTH - 20,
									TIME_BANNER_HEIGHT - 20));
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	/* Write time */
    DFBCHECK(primary->DrawString(primary, radio_str, -1, screen_width/2 - 160,
								screen_height/2 - TIME_BANNER_HEIGHT/2, DSTF_LEFT));
}

static void draw_boot_screen_fcn() 
{
/* Black screen */
	DFBCHECK(primary->SetColor(primary, 0x00, 0x00, 0x00, 0xff));
	DFBCHECK(primary->FillRectangle(primary, 0, 0, screen_width, screen_height));

	/* Make banner background */
	DFBCHECK(primary->SetColor(primary, 0x38, 0x8E, 0x3C, 0xff));
	DFBCHECK(primary->FillRectangle(primary, screen_width/2 - TIME_BANNER_WIDTH/2,
									 screen_height/2 - TIME_BANNER_HEIGHT,
									 TIME_BANNER_WIDTH,
									 TIME_BANNER_HEIGHT));

	DFBCHECK(primary->SetColor(primary, 0x4C, 0xAF, 0x50, 0xff));
	DFBCHECK(primary->FillRectangle(primary, screen_width/2 - TIME_BANNER_WIDTH/2 + 10,
									 screen_height/2 - TIME_BANNER_HEIGHT + 10,
									TIME_BANNER_WIDTH - 20,
									TIME_BANNER_HEIGHT - 20));
	DFBCHECK(primary->SetColor(primary, 0xff, 0xff, 0xff, 0xff));
	/* Write time */
    DFBCHECK(primary->DrawString(primary, "Booting...", -1, screen_width/2 - 150,
								screen_height/2 - TIME_BANNER_HEIGHT/2, DSTF_LEFT));

}

/* Callbacks called when timers expire to clear drawing flags */
static void clr_channel_info_flag()
{
	draw_channel_info_flag = 0;
}

static void clr_time_flag()
{
	draw_time_flag = 0;
}

static void clr_volume_flag()
{
    draw_volume_flag = 0;
}

static int8_t load_volume_images()
{
    int8_t i;
    char image_path[30];
    char sprintf_conversion[30];

    strcpy(sprintf_conversion, image_folder);
    strcat(sprintf_conversion, "volume_%0d.png");

    for (i = 0; i < VOLUME_IMAGE_NO; i++)
    {
        sprintf(image_path, sprintf_conversion, i);
        printf("image path: %s\n", image_path);
        DFBCHECK(dfbInterface->CreateImageProvider(dfbInterface, image_path, &(volume_image_provider[i])));
        DFBCHECK(volume_image_provider[i]->GetSurfaceDescription(volume_image_provider[i], &surfaceDesc));
        DFBCHECK(dfbInterface->CreateSurface(dfbInterface, &surfaceDesc, &(volume_image_surface[i])));
        DFBCHECK(volume_image_provider[i]->RenderTo(volume_image_provider[i], volume_image_surface[i], NULL));
        volume_image_provider[i]->Release(volume_image_provider[i]);
    }

    return NO_ERR;
}

static void format_time_str(tdt_time_t time, char* str)
{
    char hour_str[4];
    char minute_str[3];
    /* AM */
    if (time.hour >= 0 && time.hour < 12)
    {
        strcpy(str, "AM ");
        if (time.hour == 0)
        {
            time.hour = 12;
        }
        if (time.hour < 10)
        {
            strcat(str, "0");
        }
        sprintf(hour_str, "%d:", time.hour);
        strcat(str, hour_str);
    /* PM */
    } else
    {
        strcpy(str, "PM ");
        if (time.hour > 12)
        {
         time.hour -= 12;
        }
        if (time.hour < 10)
        {
            strcat(str, "0");
        }
        sprintf(hour_str, "%d:", time.hour);
        strcat(str, hour_str);
    }

    /* Add minutes to string which will be written by render thread */
    if (time.minute < 10)
    {
        sprintf(minute_str, "0%d", time.minute);
    } else
    {
        sprintf(minute_str, "%d", time.minute);
    }

    strcat(str, minute_str);
    printf("Time: %s", str);
}
