#include "remote_controller.h"
#include "graphic_controller.h"
#include "stream_controller.h"
#include "init_parser.h"
#include "tdp_api.h"
#include <semaphore.h>
#include <time.h>
#include <pthread.h>

/* Struct used to repesent app state */
struct app_state
{
    int8_t app_running;
    channel_t current_channel;
    int8_t volume_level;
	int8_t volume_muted;
	int8_t tdt_collected;
	time_t start_time_sys;
	tdt_time_t start_time_tdt;
};

static struct app_state stb_state;
static sem_t semaphore_channel;
static const char init_file_path[] = "./init.ini";
static int8_t requested_channel;

/* Helper function used to add time on time which is received from TDT */
static tdt_time_t add_sys_time(tdt_time_t stb_time, uint8_t hour_offset, uint8_t min_offset);
/* Helper function used to check if all values from init_file are read */
static int8_t check_init_values(int32_t* init_freq, int32_t* init_band, int32_t* init_video_pid,
							int32_t* init_audio_pid, int32_t* init_p_no, enum t_Module* init_mod,
     						 enum t_StreamType* init_vtype, enum t_StreamType* init_atype);
/* Callback used to decode keypress from remote */
static void decode_keypress(uint16_t keycode);

int32_t main()
{
    int8_t status;
	int32_t init_freq;
	int32_t init_band;
	int32_t init_video_pid;
	int32_t init_audio_pid;
	int32_t init_program_no;
	enum t_Module init_modulation;
	enum t_StreamType init_video_type;
	enum t_StreamType init_audio_type;
	char* str;
	pthread_t thread;

	sem_init(&semaphore_channel, 0, 0);

/* Reading from init file */
	init_file_parse(init_file_path);
	status = check_init_values(&init_freq, &init_band, &init_video_pid, &init_audio_pid,
							&init_program_no, &init_modulation, &init_video_type, &init_audio_type);
	if (status == -1)
	{
		return -1;
	}
	printf("********************************************** \n");
/* Configure STB */
    stb_state.app_running = 1;
    stb_state.current_channel.channel_no = init_program_no;
	stb_state.current_channel.audio_pid = init_audio_pid;
	stb_state.current_channel.video_pid = init_video_pid;
	stb_state.volume_muted = 0;
	stb_state.tdt_collected = 0;
	if (init_video_pid == -1)
	{
		stb_state.current_channel.has_video = 0;
	} else 
	{
		stb_state.current_channel.has_video = 1;
	}
	stb_state.volume_level = 5;
/* Init tuner */
	status = tuner_init(init_freq);
    if (status == ERR)
    {
		tuner_deinit();
		return ERR;
	}
/* Init graphic */
	status = graphic_init();
	if (status == ERR)
	{
		remote_deinit();
		tuner_deinit();
		return ERR;
	}
/* Show booting screen */
	graphic_draw_boot_screen();
/* Get start time */
	//stb_state.start_time_tdt = player_get_time();
	time(&stb_state.start_time_sys);
/* Remove booting screen */
	graphic_remove_boot_screen();
/* Play intit channel, PMT */
	status = player_play_init_channel(&stb_state.current_channel, init_video_type, init_audio_type);
	if (status == ERROR)
	{
		printf("Wrong audio or video pid for channel %d\n", init_program_no);
		status = graphic_deinit();
		status = tuner_deinit();
		return ERR;
	}
	player_set_volume(stb_state.volume_level);
	graphic_draw_channel_info(stb_state.current_channel);
/* Init remote */	
	remote_set_decode_keypress(decode_keypress);
    status = remote_init();
	if (status == ERR)
	{
		tuner_deinit();
		graphic_deinit();
		return ERR;
	}

/* Loop to check changing channel */	
	while (stb_state.app_running) {
        sem_wait(&semaphore_channel);
    	if (stb_state.app_running == 0)
		{
			break;
		}
		if (requested_channel != stb_state.current_channel.channel_no)
		{
			stb_state.current_channel.channel_no = requested_channel;
			player_play_channel(&stb_state.current_channel);
			usleep(2000000);
		}
		graphic_draw_channel_info(stb_state.current_channel);
	}

	status = graphic_deinit();
    status = remote_deinit();
    status = tuner_deinit();

	return 0;
}

static void decode_keypress(uint16_t keycode)
{
    time_t raw_time;
	struct tm* current_time_offset;
	tdt_time_t current_time;
	int8_t channel_remote;

	switch (keycode)
    {
        case KEYCODE_INFO:
            printf("Currently on channel %d\n", stb_state.current_channel.channel_no);
			printf("Channel video PID: %d \n", stb_state.current_channel.video_pid);
			printf("Channel audio PID: %d \n", stb_state.current_channel.audio_pid);
			printf("Channel has teletext %d \n", stb_state.current_channel.has_teletext);
			stb_state.start_time_tdt = player_get_time();
            time(&raw_time);
/* Calculate time offset from systime */
			raw_time -= stb_state.start_time_sys;
			current_time_offset = localtime(&raw_time);
			current_time = add_sys_time(stb_state.start_time_tdt, current_time_offset->tm_hour, current_time_offset->tm_min);
			printf("%d hour %d minute %d sec \n", current_time_offset->tm_hour, current_time_offset->tm_min, current_time_offset->tm_sec);

			graphic_draw_time(current_time);
			graphic_draw_channel_info(stb_state.current_channel);
			break;
        case KEYCODE_P_PLUS:
            if (stb_state.current_channel.channel_no + 1 > MAX_CHANNEL)
            {
                requested_channel = MIN_CHANNEL;
            } else 
			{
				requested_channel = stb_state.current_channel.channel_no + 1;
			}
			sem_post(&semaphore_channel);
            break;
        case KEYCODE_P_MINUS:
            if (stb_state.current_channel.channel_no - 1 < MIN_CHANNEL)
            {
                requested_channel = MAX_CHANNEL;
            } else
			{
				requested_channel = stb_state.current_channel.channel_no - 1;
			}
			sem_post(&semaphore_channel);
            break;
        case KEYCODE_VOL_PLUS:
            if (++stb_state.volume_level > MAX_VOL_LEVEL)
            {
                stb_state.volume_level = MAX_VOL_LEVEL;
            }
            graphic_draw_volume_level(stb_state.volume_level);
            player_set_volume(stb_state.volume_level);
			break;
        case KEYCODE_VOL_MINUS:
            if (--stb_state.volume_level < MIN_VOL_LEVEL)
            {
                stb_state.volume_level = MIN_VOL_LEVEL;
            }
            graphic_draw_volume_level(stb_state.volume_level);
            player_set_volume(stb_state.volume_level);
			break;
        case KEYCODE_VOL_MUTE:
			if (stb_state.volume_muted == 0)
			{
				stb_state.volume_muted = 1;
            	graphic_draw_volume_level(MIN_VOL_LEVEL);
            	player_set_volume(MIN_VOL_LEVEL);
			} else 
			{
				stb_state.volume_muted = 0;
            	graphic_draw_volume_level(stb_state.volume_level);
            	player_set_volume(stb_state.volume_level);
			}
			break;
        case KEYCODE_EXIT:
            stb_state.app_running = 0;
			sem_post(&semaphore_channel);
            break;
		case KEYCODE_P_1 ... KEYCODE_P_6:
			requested_channel = keycode - 1;
			if (requested_channel != stb_state.current_channel.channel_no)
			{
				sem_post(&semaphore_channel);
			} else
			{
				graphic_draw_channel_info(stb_state.current_channel);
			}
			break;
		case KEYCODE_P_0:
			requested_channel = 0;
			if (requested_channel != stb_state.current_channel.channel_no)
			{
				sem_post(&semaphore_channel);
			} else
			{
				graphic_draw_channel_info(stb_state.current_channel);
			}	
        default:
            printf("Press P+, P-, INFO or EXIT !\n");
    }
}

static int8_t check_init_values(int32_t* init_freq, int32_t* init_band, int32_t* init_video_pid,
							int32_t* init_audio_pid, int32_t* init_p_no, enum t_Module* init_mod,
     						 enum t_StreamType* init_vtype, enum t_StreamType* init_atype)
{
	char* str;
	
/* Check if frequency is read */
	*init_freq = parser_get_frequency();
	if (*init_freq == -1)
	{
		printf("\"frequency\" not found in config \n");
		return -1;
	}
	printf("Frequency read from init file: %d \n", *init_freq);
/* Check if bandwidth is read */
	*init_band = parser_get_bandwidth();
	if (*init_band == -1)
	{
		printf("\"bandwidth\" not found in config \n");
		return -1;
	}
	printf("Bandwidth read from init file: %d \n", *init_band);
/* Check if modulation is read */
	str = parser_get_modulation();
	if (str == NULL)
	{
		printf("\"modulation\" not found in config \n");
	} else
	{
		if (strcmp(str, "DVB_T\n") == 0)
		{
			*init_mod = DVB_T;
		} else if (strcmp(str, "DVB_T2\n") == 0) 
		{
			*init_mod = DVB_T2;
		} else 
		{
			printf("Unknown option %s for modulation\n");
			return -1;
		}
	}
	printf("Modulation read from init file: %s", str);
/* Check if video pid is read */
	*init_video_pid = parser_get_video_pid();
	if (*init_video_pid == -1)
	{
		printf("\"video_pid\" not found in config \n");
		return -1;
	}
	printf("Video_pid read from init file: %d \n", *init_video_pid);
/* Check if audio pid is read */
	*init_audio_pid = parser_get_audio_pid();
	if (*init_audio_pid == -1)
	{
		printf("\"audio_pid\" not found in config \n");
		return -1;
	
	}
	printf("Audio_pid read from init file: %d \n", *init_audio_pid);
/* Check if video type is read */
	str = parser_get_video_type();
	if (str == NULL)
	{
		printf("\"video_type\" not found in config \n");
	} else
	{
		if (strcmp(str, "VIDEO_TYPE_MPEG2\n") == 0)
		{
			*init_vtype = VIDEO_TYPE_MPEG2;
		} else 
		{
			printf("Unknown option %s for video type\n");
			return -1;
		}
	}
	printf("Video_type read from init file: %s", str);
/* Check if audio type is read */
	str = parser_get_audio_type();
	if (str == NULL)
	{
		printf("\"audio_type\" not found in config \n");
	} else
	{
		if (strcmp(str, "AUDIO_TYPE_MPEG_AUDIO\n") == 0)
		{
			*init_atype = AUDIO_TYPE_MPEG_AUDIO;
		} else 
		{
			printf("Unknown option %s for audio type\n");
			return -1;
		}
	}
	printf("Audito_type read from init file: %s", str);
/* Check if program no is read */
	*init_p_no = parser_get_program_number();
	if (*init_p_no == -1)
	{
		printf("\"program_number\" not found in config \n");
		return -1;
	} else
	{
		if (*init_p_no < MIN_CHANNEL || *init_p_no > MAX_CHANNEL)
		{
			printf("program number %d too high!\n", *init_p_no);
			return -1;
		}
	}
	printf("Program_number read from init file: %d \n", *init_p_no);
	
	return 0;	
}
static tdt_time_t add_sys_time(tdt_time_t stb_time, uint8_t hour_offset, uint8_t min_offset)
{
	stb_time.hour += hour_offset;
	if (stb_time.hour > 23)
	{
		stb_time.hour -= 24;
	}

	stb_time.minute += min_offset;
	if (stb_time.minute > 59)
	{
		stb_time.minute -= 60;
		if (++stb_time.hour > 23)
		{
			stb_time.hour = 0;
		}
	}

	return stb_time;
}
