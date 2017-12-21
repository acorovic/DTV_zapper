#include "stream_controller.h"

static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static uint32_t player_handle;
static uint32_t source_handle;
static uint32_t filter_handle;
static uint32_t video_handle;
static uint32_t audio_handle;

static int8_t video_running = 0;
static int8_t audio_running = 0;

static int8_t tdt_filtering_set = 0;
static int8_t tot_filtering_set = 0;

/* Array which stores channel infos */
static service_t service_info_array[10];
/* Struct which stores channel specific info */
static pmt_t pmt_info;
/* Struct which stores time info */
static tdt_time_t tdt_time;


/* Tuner callback */
static int32_t tuner_callback(t_LockStatus status);

int8_t player_play_channel(channel_t* channel)
{
	t_Error status;
	service_t channel_info;
	pmt_t channel_pmt;

	if (audio_running == 1)
    {
		Player_Stream_Remove(player_handle, source_handle, audio_handle);
		audio_running = 0;
	}

	if (video_running == 1)
    {
		Player_Stream_Remove(player_handle, source_handle, video_handle);
		video_running = 0;
	}

	if (parser_get_time_completed() == 0 && tdt_filtering_set == 1)
	{
		printf("Stopped TDT parsing \n");
		stop_tdt_parsing();
		tdt_filtering_set = 0;
	} else if (parser_get_timezone_completed() == 0 && tot_filtering_set == 1)
	{
		printf("Stopped TOT parsing \n");
		stop_tot_parsing();
		tot_filtering_set = 0;
	}

	channel_info = service_info_array[channel->channel_no];
	filter_pmt(channel_info.pid, &pmt_info);
	if (parser_get_time_completed() == 0 && tdt_filtering_set == 0)
	{
		printf("Started TDT parsing \n");
		start_tdt_parsing();
		tdt_filtering_set = 1;
	} else if (parser_get_timezone_completed() == 0 && tot_filtering_set == 0)
	{
		printf("Started TOT parsing \n");
		start_tot_parsing();
		tot_filtering_set = 1;
	}

	channel_pmt = pmt_info;

	if (channel_pmt.has_video == 1)
	{
		status = Player_Stream_Create(player_handle, source_handle, 
									channel_pmt.video_pid, VIDEO_TYPE_MPEG2,
									&video_handle);
		ASSERT_TDP_RESULT(status, "video stream");
		channel->video_pid = channel_pmt.video_pid;
		channel->has_video = 1;
		video_running = 1;
	} else 
	{
		channel->has_video = 0;
	}

	status = Player_Stream_Create(player_handle, source_handle,
								channel_pmt.audio_pid[0], AUDIO_TYPE_MPEG_AUDIO,
								&audio_handle);
	ASSERT_TDP_RESULT(status, "audio stream");
	channel->audio_pid = channel_pmt.audio_pid[0];

	if (channel_pmt.has_teletext)
	{
		channel->has_teletext = 1;
	}

	audio_running = 1;

	return NO_ERROR;
}

int8_t player_play_init_channel(channel_t* channel, enum t_StreamType video_type, enum t_StreamType audio_type)
{
	t_Error status;
	service_t channel_info;
	pmt_t channel_pmt;

    /* Filter PAT when playing channel first time */
    status = filter_pat(service_info_array);
    if (status == ERROR)
    {
        return ERROR;
    }
    /* Get channel PID from received PAT data */
	channel_info = service_info_array[channel->channel_no];
	/* Filter PMT */
    filter_pmt(channel_info.pid, &pmt_info);
	channel_pmt = pmt_info;

	if (channel_pmt.audio_pid[0] != channel->audio_pid)
	{
		audio_running = 0;
		return ERROR;
	}

	if (channel_pmt.has_video == 1)
	{
/* Check if video PID read from init file is the same as the one from the PMT for program no */
		if (channel->video_pid == channel_pmt.video_pid)
		{
			status = Player_Stream_Create(player_handle, source_handle, 
										channel_pmt.video_pid, video_type,
										&video_handle);
			ASSERT_TDP_RESULT(status, "video stream");
			channel->video_pid = channel_pmt.video_pid;
			channel->has_video = 1;
			video_running = 1;
		} else
		{
			video_running = 0;
			return ERROR;
		}
	} else 
	{
		channel->has_video = 0;
		video_running = 0;
	}

	status = Player_Stream_Create(player_handle, source_handle,
								channel->audio_pid, audio_type,
								&audio_handle);
	// Add guard to close video stream if audio stream failed!!!
	ASSERT_TDP_RESULT(status, "audio stream");
	channel->audio_pid = channel_pmt.audio_pid[0];

	if (channel_pmt.has_teletext)
	{
		channel->has_teletext = 1;
	}

	audio_running = 1;

	return NO_ERROR;
}

tdt_time_t player_get_time() {
	if (parser_get_time_completed() == 0 && tdt_filtering_set == 1)
	{
		printf("Stopped TDT parsing \n");
		stop_tdt_parsing();
		tdt_filtering_set = 0;
	} else if (parser_get_timezone_completed() == 0 && tot_filtering_set == 1)
	{
		printf("Stopped TOT parsing \n");
		stop_tot_parsing();
		tot_filtering_set = 0;
	}
	if (parser_get_time_completed() == 0 && tdt_filtering_set == 0)
	{
		printf("Started TDT parsing \n");
		start_tdt_parsing();
		tdt_filtering_set = 1;
	} else if (parser_get_timezone_completed() == 0 && tot_filtering_set == 0)
	{
		printf("Started TOT parsing \n");
		start_tot_parsing();
		tot_filtering_set = 1;
	}

	return parser_get_time();
}

int8_t player_set_volume(uint8_t vol_level)
{
	t_Error status;

	if (audio_running)
	{
		status = Player_Volume_Set(player_handle, vol_level * 160400000);
		ASSERT_TDP_RESULT(status, "set volume");
	}

	return NO_ERR;
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
	pmt_info.has_teletext = 0;
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


