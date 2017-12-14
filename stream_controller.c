#include "stream_controller.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_pat = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_pmt = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_tdt = PTHREAD_COND_INITIALIZER;

static uint32_t player_handle;
static uint32_t source_handle;
static uint32_t filter_handle;
static uint32_t video_handle;
static uint32_t audio_handle;

static int8_t video_running = 0;
static int8_t audio_running = 0;
static int8_t time_read_success = 0;

static service_t service_info_array[10];
static pmt_t pmt_info;
static tdt_time_t tdt_time;

static int32_t pat_filter_callback(uint8_t* buffer)
{
	uint16_t section_length;
	uint16_t program_section_length;
	uint16_t n = 0;
	uint16_t program_no;
	uint16_t pid;
	uint8_t* program_section_ptr;
	uint8_t i = 0;

	printf("PAT arrived \n");

	section_length = (uint16_t) (
					(*(buffer+1) << 8) + (*(buffer+2))
					& 0x0fff);

	printf("Section length %d \n", section_length);

	program_section_length = section_length - 9;
	n = program_section_length;
	program_section_ptr = (buffer + 8);

	while (n > 0)
    {
		program_no = (uint16_t) (*(program_section_ptr) << 8)
								+ (*(program_section_ptr + 1));

		if (program_no != 0)
        {
			pid = (uint16_t) ((*(program_section_ptr + 2) << 8)
							 + (*(program_section_ptr + 3))
							 & 0x1fff);
			printf("Program number %d, PID: %d \n", program_no, pid);
			service_info_array[i].program_no = program_no;
			service_info_array[i].pid = pid;
			i++;
		}
		n -= 4;
		if (n > 0)
        {
			program_section_ptr += 4;
        }
	}
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&condition_pat);
	pthread_mutex_unlock(&mutex);
	printf("Pat table parsed \n");
}

static int32_t pmt_filter_callback(uint8_t* buffer)
{
   	uint16_t section_length;
	uint16_t program_section_length;
	uint16_t n = 0;
	uint16_t program_no;
	uint16_t pid;
	uint8_t* program_section_ptr;
	uint8_t i = 0;

    printf("PMT arrived \n");
    section_length = (uint16_t) (
                    (*(buffer + 1) << 8) + (*(buffer + 2))
                    & 0x0fff);

    uint16_t program_info_length = (uint16_t) (
                                (*(buffer + 10) << 8) + (*(buffer + 11))
                                & 0x0fff);

    program_section_length = section_length - 4 - (program_info_length + 9);

    printf("Program info length %d \nProgram section length %d \n",
    program_info_length, program_section_length);
    n = program_section_length;
    program_section_ptr = buffer + 12 + program_info_length;

    while (n > 0)
    {
        uint8_t stream_type;
        uint16_t elementary_PID;
        uint16_t ES_info_length;
        stream_type = *program_section_ptr;
        elementary_PID = (uint16_t) (
                         (*(program_section_ptr + 1) << 8) + (*(program_section_ptr + 2))
                         & 0x1fff);
        ES_info_length = (uint16_t) (
                          (*(program_section_ptr + 3) << 8) + (*(program_section_ptr + 4))
                         & 0x0fff);
        printf("Stream type %d, elementary PID %d, es_info %d\n", stream_type, elementary_PID, ES_info_length);
        if (stream_type == 2)
        {
            pmt_info.video_pid = elementary_PID;
        	pmt_info.has_video = 1;
		} else if (stream_type == 3)
        {
            pmt_info.audio_pid[0] = elementary_PID;
        } else if (stream_type == 4)
        {
            pmt_info.audio_pid[1] = elementary_PID;
        }
        n -= (5 + ES_info_length);
        if (n > 0)
        {
            program_section_ptr += (5 + ES_info_length);
        }
    }
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&condition_pmt);
    pthread_mutex_unlock(&mutex);
    printf("Pmt table parsed \n");

	return NO_ERR;
}

static int32_t tdt_filter_callback(uint8_t* buffer)
{
	uint16_t MJD = 0;
	uint32_t UTC = 0;

	printf("TDT arrived \n");

	MJD = (uint16_t) (
		  (*(buffer+3) << 8) + (*(buffer+4)));

	UTC = (uint32_t) (
		  (*(buffer+5) << 16) + (*(buffer+6) << 8) + ((*buffer+7)));

	printf("MJD: %d, UTC %d \n");

	tdt_time.minute = (uint8_t) ((UTC & 0xff00) >> 8);
	tdt_time.hour = (uint8_t) ((UTC & 0xff0000) >> 16);

	printf("minute: %d\n", tdt_time.minute);

	time_read_success = 1;
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&condition_tdt);
	pthread_mutex_unlock(&mutex);
	printf("TDT table parsed \n");

	return NO_ERR;
}

static int32_t tuner_callback(t_LockStatus status)
{
	if (status == STATUS_LOCKED)
    {
		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&condition);
		pthread_mutex_unlock(&mutex);
		printf("Callback locked \n");
	} else
    {
		printf("Callback not locked \n");
	}

	return 0;
}

int8_t play_channel(int8_t channel_no)
{
	t_Error status;
	service_t channel_info;
	pmt_t channel_pmt;

	if (audio_running == 1)
    {
		Player_Stream_Remove(player_handle, source_handle, audio_handle);
	}

	if (video_running == 1)
    {
		Player_Stream_Remove(player_handle, source_handle, video_handle);
	}

	channel_info = service_info_array[channel_no];
	filter_pmt(channel_info.pid);
	channel_pmt = pmt_info;

	if (channel_pmt.has_video == 1)
	{
		status = Player_Stream_Create(player_handle, source_handle, 
									channel_pmt.video_pid, VIDEO_TYPE_MPEG2,
									&video_handle);
		ASSERT_TDP_RESULT(status, "video stream");
		video_running = 1;
	}
	status = Player_Stream_Create(player_handle, source_handle,
								channel_pmt.audio_pid[0], AUDIO_TYPE_MPEG_AUDIO,
								&audio_handle);
	ASSERT_TDP_RESULT(status, "audio stream");
	audio_running = 1;

	return NO_ERROR;
}

tdt_time_t get_time() {
	tdt_time_t ret_val = {99, 99};

	time_read_success = 0;
	filter_tdt();

	if (time_read_success)
    {
		ret_val = tdt_time;
	}
	return ret_val;
}

int8_t tuner_init(uint32_t frequency)
{
	t_Error status;
	struct timeval now;
	struct timespec time_to_wait;
	int8_t rt;
	int8_t i;
	int8_t j;

	for (i = 0; i < 20; i++)
	{
		service_info_array[i].program_no = 0;
		service_info_array[i].pid = 0;
	}

	pmt_info.has_video = 0;
	pmt_info.video_pid = 0;
	for (i = 0; i < 4; i++)
	{
		pmt_info.audio_pid[i] = 0;
	}

	status = Tuner_Init();
	ASSERT_TDP_RESULT(status, "tuner init");

	status = Tuner_Register_Status_Callback(tuner_callback);
	ASSERT_TDP_RESULT(status, "tuner register callback");

	status = Tuner_Lock_To_Frequency(frequency, 8, DVB_T);
	ASSERT_TDP_RESULT(status, "tuner lock");


	printf("%d lock time \n");
	gettimeofday(&now, NULL);
	time_to_wait.tv_sec = now.tv_sec + LOCK_TIME;
	time_to_wait.tv_nsec = now.tv_usec + LOCK_TIME;

	pthread_mutex_lock(&mutex);
	rt = pthread_cond_timedwait(&condition, &mutex, &time_to_wait);
	if (rt == ETIMEDOUT)
    {
		printf("Lock time exceeded! \n");
		return ERROR;
	}

	pthread_mutex_unlock(&mutex);

    status = Player_Init(&player_handle);
	ASSERT_TDP_RESULT(status, "player init");

	status = Player_Source_Open(player_handle, &source_handle);
	ASSERT_TDP_RESULT(status, "player open");

	return NO_ERROR;
}

int8_t tuner_deinit()
{
	t_Error status;

	status = Tuner_Unregister_Status_Callback(tuner_callback);
	ASSERT_TDP_RESULT(status, "tuner unregister");

	status = Tuner_Deinit();
	ASSERT_TDP_RESULT(status, "tuner deinit");

   	status = Player_Source_Close(player_handle, source_handle);
	ASSERT_TDP_RESULT(status, "player close");

	status = Player_Deinit(player_handle);
	ASSERT_TDP_RESULT(status, "player deinit");

	return NO_ERROR;
}

int8_t demux_init(uint32_t PID, uint32_t tableID, int32_t (*demux_filter_callback)(uint8_t* buffer))
{
    t_Error status;

    status = Demux_Set_Filter(player_handle, PID, tableID, &filter_handle);
	ASSERT_TDP_RESULT(status, "demux set filter");

	status = Demux_Register_Section_Filter_Callback(demux_filter_callback);
	ASSERT_TDP_RESULT(status, "register section filter");

    return NO_ERROR;
}

int8_t demux_deinit(int32_t (*demux_filter_callback)(uint8_t* buffer))
{
    t_Error status;

    status = Demux_Unregister_Section_Filter_Callback(demux_filter_callback);
    ASSERT_TDP_RESULT(status, "demux untregister filter callback");

    status = Demux_Free_Filter(player_handle, filter_handle);
    ASSERT_TDP_RESULT(status, "demux free filter");

    return NO_ERROR;
}

int8_t filter_pat()
{
    int8_t rt;
    struct timeval now;
	struct timespec time_to_wait;
	t_Error status;

    status = demux_init(PAT_PID, PAT_TABLEID, pat_filter_callback);
	ASSERT_TDP_RESULT(status, "set pat demux");

    gettimeofday(&now, NULL);
	time_to_wait.tv_sec = now.tv_sec + LOCK_TIME;
	time_to_wait.tv_nsec = now.tv_usec;

    pthread_mutex_lock(&mutex);
    rt = pthread_cond_timedwait(&condition_pat, &mutex, &time_to_wait);
    if (rt == ETIMEDOUT)
    {
        printf("Couldn't parse PAT!\n");
        return ERROR;
    }
    pthread_mutex_unlock(&mutex);

    status = demux_deinit(pat_filter_callback);
    ASSERT_TDP_RESULT(status, "pat filter callback free demux");
    printf("PAT parsed \n");

    return NO_ERROR;
}

int8_t filter_pmt(uint16_t channel_pid)
{
    int8_t rt;
    struct timeval now;
	struct timespec time_to_wait;
	t_Error status;

	printf("channel pid filter_pmt %d\n", channel_pid);

    status = demux_init(channel_pid, PMT_TABLEID, pmt_filter_callback);
	ASSERT_TDP_RESULT(status, "set pmt demux");

    gettimeofday(&now, NULL);
	time_to_wait.tv_sec = now.tv_sec + LOCK_TIME;
	time_to_wait.tv_nsec = now.tv_usec + LOCK_TIME;

    pthread_mutex_lock(&mutex);
    rt = pthread_cond_timedwait(&condition_pmt, &mutex, &time_to_wait);
    if (rt == ETIMEDOUT)
    {
        printf("Couldn't parse PMT!\n");
	    return ERROR;
    }
    pthread_mutex_unlock(&mutex);

    status = demux_deinit(pmt_filter_callback);
    ASSERT_TDP_RESULT(status, "pmt filter callback free demux");
    printf("PMT parsed \n");

    return NO_ERROR;
}


int8_t filter_tdt()
{
    int8_t rt;
    struct timeval now;
	struct timespec time_to_wait;
	t_Error status;

    status = demux_init(TDT_PID, TDT_TABLEID, tdt_filter_callback);
	ASSERT_TDP_RESULT(status, "set tdt demux");

    gettimeofday(&now, NULL);
	time_to_wait.tv_sec = now.tv_sec + LOCK_TIME;
	time_to_wait.tv_nsec = now.tv_usec + LOCK_TIME;

    pthread_mutex_lock(&mutex);
    rt = pthread_cond_timedwait(&condition_tdt, &mutex, &time_to_wait);
    if (rt == ETIMEDOUT)
    {
        printf("Couldn't parse TDT!\n");
	    return ERROR;
    }
    pthread_mutex_unlock(&mutex);

    status = demux_deinit(tdt_filter_callback);
    ASSERT_TDP_RESULT(status, "tdt filter callback free demux");
    printf("TDT parsed \n");

    return NO_ERROR;
}
