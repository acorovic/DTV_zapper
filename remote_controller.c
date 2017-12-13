#include "remote_controller.h"

static const char* dev = "/dev/input/event0";
static int32_t input_file_desc;
static struct input_event* event_buf;
static pthread_t remote_thread;
static int8_t remote_thread_running = 0;
static void (*decode_keypress)(uint16_t keycode);

static int32_t get_keys(int32_t count, uint8_t* buf, int32_t* events_read)
{
  int32_t ret = 0;

  /* read input events and put them in buffer */
  ret = read(input_file_desc, buf, (size_t)(count * (int)sizeof(struct input_event)));
  if(ret <= 0)
  {
    printf("Error code %d", ret);
    return ERR;
  }
  /* calculate number of read events */
  *events_read = ret / (int)sizeof(struct input_event);
  return NO_ERR;
}

static void* input_event_task()
{
    uint32_t event_cnt;
    uint16_t i;
    while (remote_thread_running)
    {
        if (get_keys(NUM_EVENTS, (uint8_t*)event_buf, &event_cnt))
        {
            printf("Error while reading input events!\n");
            return (void*)ERR;
        }
        for (i = 0; i < event_cnt; i++)
        {
            if (event_buf[i].type == EV_KEY &&
               (event_buf[i].value == EV_VALUE_KEYPRESS || event_buf[i].value == EV_VALUE_AUTOREPEAT))
            {
                decode_keypress(event_buf[i].code);
            }
        }
    }

    return (void*) NO_ERR;
}

int8_t remote_init()
{
    char device_name[20];

    input_file_desc = open(dev, O_RDWR);
    if (input_file_desc == -1)
    {
        printf("Error while opening device (%s)! \n", strerror(errno));
        return ERR;
    }

    ioctl(input_file_desc, EVIOCGNAME(sizeof(device_name)), device_name);
    printf("Remote controller device opened succesfully!\n");

    event_buf = malloc(NUM_EVENTS * sizeof(struct input_event));
    if (!event_buf)
    {
        printf("Error allocating memory \n");
        return ERR;
    }

    remote_thread_running = 1;
    if (pthread_create(&remote_thread, NULL, &input_event_task, NULL))
    {
        remote_thread_running = 0;
        printf("Error creating thread for remote\n");
        return ERR;
    }

    return NO_ERR;
}

void remote_set_decode_keypress(void (*callback)(uint16_t keycode)) {
    decode_keypress = callback;
	printf("decode_keypress callback set \n");
}

int8_t remote_deinit() {
    remote_thread_running = 0;

    if (pthread_join(remote_thread, NULL))
    {
        printf("Error during remote_thread join!\n");
        return ERR;
    }

    free(event_buf);
    return NO_ERR;
}
