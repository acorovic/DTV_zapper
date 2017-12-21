#include "table_parser.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition_pat = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_pmt = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_tdt = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_tot = PTHREAD_COND_INITIALIZER;

static service_t service_info_array[10];

static pmt_t pmt_info;
static tdt_time_t tdt_time;

static int8_t time_read_success = 0;
static int8_t timezone_is_set = 0;
/* Callbacks */
static int32_t pat_filter_callback(uint8_t* buffer);
static int32_t pmt_filter_callback(uint8_t* buffer);
static int32_t tdt_filter_callback(uint8_t* buffer);
static int32_t tot_filter_callback(uint8_t* buffer);

static pthread_t tdt_thread;
static void* tdt_loop;

/* Helper function to change time according to TOT table offsets */
static int8_t set_timezone(tdt_time_t* utc_time, uint8_t polarity, uint8_t hour_offset, uint8_t minute_offset);

int8_t filter_pat(service_t* pat_info)
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
        demux_deinit(pat_filter_callback);
        return ERROR;
    }
    pthread_mutex_unlock(&mutex);

    status = demux_deinit(pat_filter_callback);
    ASSERT_TDP_RESULT(status, "pat filter callback free demux");
    printf("PAT parsed \n");

    /* Copy from received PAT table to parameter */
    memcpy(pat_info, &service_info_array, sizeof(service_info_array));

    return NO_ERROR;
}

int8_t filter_pmt(uint16_t channel_pid, pmt_t* channel_pmt_info)
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
        demux_deinit(pmt_filter_callback);
	    return ERROR;
    }
    pthread_mutex_unlock(&mutex);

    status = demux_deinit(pmt_filter_callback);
    ASSERT_TDP_RESULT(status, "pmt filter callback free demux");
    printf("PMT parsed \n");

    /* Copy from received PMT info to parameter */
    memcpy(channel_pmt_info, &pmt_info, sizeof(pmt_info));

    return NO_ERROR;
}


int8_t filter_tdt(tdt_time_t* player_time)
{
    int8_t rt;
	t_Error status;

	status = demux_init(TDT_PID, TDT_TABLEID, tdt_filter_callback);
	if (status == -1)
	{
		status = demux_deinit(tdt_filter_callback);
		return ERROR;
	}
	printf("TDT filter set \n");
	status = demux_deinit(tdt_filter_callback);
    
	return status;
}

int8_t filter_tot(tdt_time_t* player_time)
{
    int8_t rt;
    struct timeval now;
	struct timespec time_to_wait;
	t_Error status;

    status = demux_init(TOT_PID, TOT_TABLEID, tot_filter_callback);
	ASSERT_TDP_RESULT(status, "set TOT demux");

    gettimeofday(&now, NULL);
	time_to_wait.tv_sec = now.tv_sec + LOCK_TIME + 20;
	time_to_wait.tv_nsec = now.tv_usec + LOCK_TIME + 20;

    pthread_mutex_lock(&mutex);
    rt = pthread_cond_timedwait(&condition_tot, &mutex, &time_to_wait);
    if (rt == ETIMEDOUT)
    {
        printf("Couldn't parse TOT!\n");
	    return ERROR;
    }
    pthread_mutex_unlock(&mutex);

    status = demux_deinit(tot_filter_callback);
    ASSERT_TDP_RESULT(status, "TOT filter callback free demux");
    printf("TOT parsed \n");

    /* Copy from received TDT */
    memcpy(player_time, &tdt_time, sizeof(tdt_time));

    return NO_ERROR;
}

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


	/* reset values */
	pmt_info.video_pid = 0;
	pmt_info.has_video = 0;
	pmt_info.audio_pid[1] = 0;
	pmt_info.audio_pid[2] = 0;

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
        /* Video */
		if (stream_type == 2)
        {
            pmt_info.video_pid = elementary_PID;
        	pmt_info.has_video = 1;
		/* Audio */
		} else if (stream_type == 3)
        {
            pmt_info.audio_pid[0] = elementary_PID;
        /* Audio */
		} else if (stream_type == 4)
        {
            pmt_info.audio_pid[1] = elementary_PID;
        /* Teletext */
		} else if (stream_type == 6)
		{
			pmt_info.has_teletext = 1;
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

int8_t parser_get_time_completed() {
	return time_read_success;
}

int8_t parser_get_timezone_completed() {
	return timezone_is_set;
}

void stop_tdt_parsing() {
	demux_deinit(tdt_filter_callback);
}

void start_tdt_parsing() {
	demux_init(TDT_PID, TDT_TABLEID, tdt_filter_callback);
}

void stop_tot_parsing() {
	demux_deinit(tot_filter_callback);
}

void start_tot_parsing() {
	demux_init(TOT_PID, TOT_TABLEID, tot_filter_callback);
}

tdt_time_t parser_get_time() {
	tdt_time.tdt_completed = time_read_success;
	return tdt_time;
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

	tdt_time.minute = (int8_t) (((UTC & 0xf000) >> 12)*10 + ((UTC & 0xf00) >> 8));
	tdt_time.hour = (int8_t) (((UTC & 0xf00000) >> 20)*10 + ((UTC & 0xf0000) >> 16));

	printf("hour: %d minute: %d\n",tdt_time.hour, tdt_time.minute);

	demux_deinit(tdt_filter_callback);
	time_read_success = 1;
	printf("TDT table parsed \n");

	return NO_ERR;
}

static int32_t tot_filter_callback(uint8_t* buffer)
{
	uint16_t descriptor_loop_len = 0;
	uint8_t* desc_ptr;
	uint8_t desc_len = 0;
	uint8_t time_offset_polarity = 0;
	uint16_t time_offset;
	uint8_t country_region_id = 0;
	uint8_t hour_offset = 0;
	uint8_t minute_offset = 0;

	printf("TOT arrived \n");

	descriptor_loop_len = (uint16_t) (
						  (*(buffer+8) << 8) + (*(buffer+9))	
						  & 0x0fff);

	printf("descriptor loop len %d\n", descriptor_loop_len);

	desc_ptr = (buffer+10);

	while (descriptor_loop_len > 0)
	{
		desc_len = *(desc_ptr+1);

		if (*(desc_ptr) == TOT_DESC)
		{
			printf("Found local time offset desc \n");
			time_offset_polarity = (uint8_t) (
								   (*(desc_ptr+5)) & 0x01);
			printf("Time offset polarity %d \n", time_offset_polarity);

			time_offset = (uint16_t) (
						  (*(desc_ptr+6) << 8) + (*(desc_ptr+7)));
		   	printf("Time offset %x \n", time_offset);

			hour_offset = time_offset >> 8;
			minute_offset = time_offset & 0x00ff;
			printf("Hour offset %d, minute offset %d \n", hour_offset, minute_offset);

			country_region_id = (uint8_t) (
								(*(desc_ptr+5) >> 2));
			printf("Country region id %d \n", country_region_id);

            if (timezone_is_set == 0)
            {
			    set_timezone(&tdt_time, time_offset_polarity, hour_offset, minute_offset);
                tdt_time.tot_completed = 1;
				timezone_is_set = 1;
				demux_deinit(tot_filter_callback);
            }
			break;
		}

		printf("Not found local time offster desc \n");
		desc_ptr += desc_len;
		descriptor_loop_len -= desc_len;
	}

	printf("TOT table parsed \n");

	return NO_ERR;
}

static int8_t set_timezone(tdt_time_t* utc_time, uint8_t polarity, uint8_t hour_offset, uint8_t minute_offset)
{
	if (polarity == 0)
	{
		printf("utc time %d hour offset %d total %d \n", utc_time->hour, hour_offset, utc_time->hour + hour_offset);
		utc_time->hour += hour_offset;
		if (utc_time->hour > 23)
		{
			utc_time->hour -= 24;
		}

		utc_time->minute += minute_offset;
		if (utc_time->minute > 59)
		{
			utc_time->minute -= 60;
            if (++utc_time->hour > 23)
            {
                utc_time->hour = 0;
            }
        }

	} else if (polarity == 1)
	{
		if (utc_time->hour - hour_offset < 0)
		{
			utc_time->hour = 24 - ((int8_t)hour_offset - utc_time->hour);
		} else
        {
            utc_time->hour -= hour_offset;
        }

		if (utc_time->minute - minute_offset < 0)
		{
			utc_time->minute = 60 - ((int8_t)minute_offset - utc_time->minute);
		    if (--utc_time->hour < 0)
            {
                utc_time->hour = 23;
            }
        } else
        {
            utc_time->minute -= minute_offset;
        }
	}
}

