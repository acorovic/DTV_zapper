#include "remote_controller.h"
#include "graphic_controller.h"
#include "stream_controller.h"
#include "init_parser.h"
#include "tdp_api.h"
#include <semaphore.h>

/* Struct used to repesent app state */
struct app_state
{
    int8_t app_running;
    channel_t current_channel;
    int8_t volume_level;
};

static struct app_state stb_state;
static sem_t semaphore_channel;
static const char init_file_path[] = "./init.ini";

/* Callback used to decode keypress from remote */
static void decode_keypress(uint16_t keycode)
{
    switch (keycode)
    {
        case KEYCODE_INFO:
            printf("Currently on channel %d\n", stb_state.current_channel.channel_no);
			printf("Channel video PID: %d \n", stb_state.current_channel.video_pid);
			printf("Channel audio PID: %d \n", stb_state.current_channel.audio_pid);
			printf("Channel has teletext %d \n", stb_state.current_channel.has_teletext);
			graphic_draw_time(player_get_time());
            graphic_draw_channel_info(stb_state.current_channel);
			break;
        case KEYCODE_P_PLUS:
            if (++stb_state.current_channel.channel_no > MAX_CHANNEL)
            {
                stb_state.current_channel.channel_no = MIN_CHANNEL;
            }
			sem_post(&semaphore_channel);
            break;
        case KEYCODE_P_MINUS:
            if (--stb_state.current_channel.channel_no < MIN_CHANNEL)
            {
                stb_state.current_channel.channel_no = MAX_CHANNEL;
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
            stb_state.volume_level = MIN_VOL_LEVEL;
            graphic_draw_volume_level(stb_state.volume_level);
            player_set_volume(stb_state.volume_level);
			break;
        case KEYCODE_EXIT:
            stb_state.app_running = 0;
			sem_post(&semaphore_channel);
            break;
        default:
            printf("Press P+, P-, INFO or EXIT !\n");
    }
}

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

	sem_init(&semaphore_channel, 0, 0);

/* Reading from init file */
	init_file_parse(init_file_path);
/* Check if frequency is read */
	init_freq = parser_get_frequency();
	if (init_freq == -1)
	{
		printf("\"frequency\" not found in config \n");
		return -1;
	}
	printf("Frequency read from init file: %d \n", init_freq);
/* Check if bandwidth is read */
	init_band = parser_get_bandwidth();
	if (init_band == -1)
	{
		printf("\"bandwidth\" not found in config \n");
		return -1;
	}
	printf("Bandwidth read from init file: %d \n", init_band);
/* Check if modulation is read */
	str = parser_get_modulation();
	if (str == NULL)
	{
		printf("\"modulation\" not found in config \n");
	} else
	{
		if (strcmp(str, "DVB_T\n") == 0)
		{
			init_modulation = DVB_T;
		} else if (strcmp(str, "DVB_T2\n") == 0) 
		{
			init_modulation = DVB_T2;
		} else 
		{
			printf("Unknown option %s for modulation\n");
			return -1;
		}
	}
	printf("Modulation read from init file: %s", str);
/* Check if video pid is read */
	init_video_pid = parser_get_video_pid();
	if (init_video_pid == -1)
	{
		printf("\"video_pid\" not found in config \n");
		return -1;
	}
	printf("Video_pid read from init file: %d \n", init_video_pid);
/* Check if audio pid is read */
	init_audio_pid = parser_get_audio_pid();
	if (init_audio_pid == -1)
	{
		printf("\"audio_pid\" not found in config \n");
		return -1;
	
	}
	printf("Audio_pid read from init file: %d \n", init_audio_pid);
/* Check if video type is read */
	str = parser_get_video_type();
	if (str == NULL)
	{
		printf("\"video_type\" not found in config \n");
	} else
	{
		if (strcmp(str, "VIDEO_TYPE_MPEG2\n") == 0)
		{
			init_video_type = VIDEO_TYPE_MPEG2;
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
			init_audio_type = AUDIO_TYPE_MPEG_AUDIO;
		} else 
		{
			printf("Unknown option %s for audio type\n");
			return -1;
		}
	}
	printf("Audito_type read from init file: %s", str);
/* Check if program no is read */
	init_program_no = parser_get_program_number();
	if (init_program_no == -1)
	{
		printf("\"program_number\" not found in config \n");
		return -1;
	} else
	{
		if (init_program_no < MIN_CHANNEL || init_program_no > MAX_CHANNEL)
		{
			printf("program number %d too high!\n", init_program_no);
			return -1;
		}
	}
	printf("Program_number read from init file: %d \n", init_program_no);
	printf("********************************************** \n");
/* Configure STB */
    stb_state.app_running = 1;
    stb_state.current_channel.channel_no = init_program_no;
	stb_state.current_channel.audio_pid = init_audio_pid;
	stb_state.current_channel.video_pid = init_video_pid;
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
/* Filter PAT */
	filter_pat();
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
		player_play_channel(&stb_state.current_channel);
		graphic_draw_channel_info(stb_state.current_channel);
	}

	status = graphic_deinit();
    status = remote_deinit();
    status = tuner_deinit();

	return 0;
}
