#include "remote_controller.h"
#include "graphic_controller.h"
#include "stream_controller.h"
/* Struct used to repesent app state */
struct app_state{
    int8_t app_running;
    int8_t current_channel;
};

static struct app_state stb_state;

/* Callback used to decode keypress from remote */
static void decode_keypress(uint16_t keycode)
{
    switch (keycode)
    {
        case KEYCODE_INFO:
            printf("Currently on channel %d\n", stb_state.current_channel);
			graphic_draw_time(get_time());
            break;
        case KEYCODE_P_PLUS:
            if (++stb_state.current_channel > MAX_CHANNEL)
            {
                stb_state.current_channel = MIN_CHANNEL;
            }
			play_channel(stb_state.current_channel);
			graphic_draw_channel_no(stb_state.current_channel);
            break;
        case KEYCODE_P_MINUS:
            if (--stb_state.current_channel < MIN_CHANNEL)
            {
                stb_state.current_channel = MAX_CHANNEL;
            }
			play_channel(stb_state.current_channel);
            break;
        case KEYCODE_EXIT:
            stb_state.app_running = 0;
            break;
        default:
            printf("Press P+, P-, INFO or EXIT !\n");
    }
}

int32_t main() {
    int8_t status;

    stb_state.app_running = 1;
    stb_state.current_channel = MIN_CHANNEL;

	status = tuner_init(754000000);
    if(status == ERR) {
		tuner_deinit();
		return ERR;
	}
	remote_set_decode_keypress(decode_keypress);
    status = remote_init();
	status = graphic_init();	
	filter_pat();
    
	play_channel(0);
	graphic_draw_channel_no(0);
	while (stb_state.app_running) {
        
    }

	status = graphic_deinit();	
    status = remote_deinit();
    status = tuner_deinit();
    return 0;
}
