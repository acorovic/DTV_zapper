#include "remote_controller.h"
#include "graphic_controller.h"
#include "stream_controller.h"

/* Struct used to repesent app state */
struct app_state
{
    int8_t app_running;
    int8_t current_channel;
    uint8_t volume_level;
};

static struct app_state stb_state;

/* Callback used to decode keypress from remote */
static void decode_keypress(uint16_t keycode)
{
    switch (keycode)
    {
        case KEYCODE_INFO:
            printf("Currently on channel %d\n", stb_state.current_channel);
			graphic_draw_time(player_get_time());
            break;
        case KEYCODE_P_PLUS:
            if (++stb_state.current_channel > MAX_CHANNEL)
            {
                stb_state.current_channel = MIN_CHANNEL;
            }
			player_play_channel(stb_state.current_channel);
			graphic_draw_channel_no(stb_state.current_channel);
            break;
        case KEYCODE_P_MINUS:
            if (--stb_state.current_channel < MIN_CHANNEL)
            {
                stb_state.current_channel = MAX_CHANNEL;
            }
			player_play_channel(stb_state.current_channel);
			graphic_draw_channel_no(stb_state.current_channel);
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
            break;
        default:
            printf("Press P+, P-, INFO or EXIT !\n");
    }
}

int32_t main()
{
    int8_t status;

    stb_state.app_running = 1;
    stb_state.current_channel = MIN_CHANNEL;
	stb_state.volume_level = 5;

	status = tuner_init(754000000);
    if (status == ERR)
    {
		tuner_deinit();
		return ERR;
	}
	remote_set_decode_keypress(decode_keypress);
    status = remote_init();
	status = graphic_init();
	filter_pat();

	player_play_channel(0);
	player_set_volume(stb_state.volume_level);
	graphic_draw_channel_no(0);
	while (stb_state.app_running) {
        
    }

	status = graphic_deinit();
    status = remote_deinit();
    status = tuner_deinit();
    return 0;
}
