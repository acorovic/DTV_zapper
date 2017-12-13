#include "remote_controller.h"
#include "common.h"

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
            break;
        case KEYCODE_P_PLUS:
            if (++stb_state.current_channel > MAX_CHANNEL)
            {
                stb_state.current_channel = MIN_CHANNEL;
            }
            break;
        case KEYCODE_P_MINUS:
            if (--stb_state.current_channel < MIN_CHANNEL)
            {
                stb_state.current_channel = MAX_CHANNEL;
            }
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

    status = tuner_init();
    remote_set_decode_keypress(decode_keypress);
    status = remote_init();
    filter_pat();

    while (stb_state.app_running) {
        
    }

    status = remote_deinit();
    status = tuner_deinit();
    return 0;
}
