#include "stream_controller.h"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_pat = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condition_pmt = PTHREAD_COND_INITIALIZER;

static uint32_t player_handle;
static uint32_t source_handle;
static uint32_t filter_handle;

int8_t tuner_init(uint32_t frequency) {
	t_Error status;
	struct timeval now;
	struct timespec time_to_wait;
	int8_t rt;

	status = Tuner_Init();
	ASSERT_TDP_RESULT(status, "tuner init");

	status = Tuner_Register_Status_Callback(tuner_callback);
	ASSERT_TDP_RESULT(status, "tuner register callback");

	status = Tuner_Lock_To_Frequency(frequency, 8, DVB_T);
	ASSERT_TDP_RESULT(status, "tuner lock");

	gettimeofday(&now, NULL);
	time_to_wait.tv_sec = now.tv_sec + LOCK_TIME;
	time_to_wait.tv_nsec = now.tv_usec;

	pthread_mutex_lock(&mutex);
	rt = pthread_cond_timedwait(&condition, &mutex, &time_to_wait);
	if(rt == ETIMEDOUT) {
		printf("Lock time exceeded! \n");
		return ERROR;
	}
	
	pthread_mutex_unlock(&mutex);

	return NO_ERROR;
}

int8_t tuner_deinit() {
	t_Error status;

	status = Tuner_Unregister_Status_Callback(tuner_callback);
	ASSERT_TDP_RESULT(status, "tuner unregister");

	status = Tuner_Deinit();
	ASSERT_TDP_RESULT(status, "tuner deinit");

	return NO_ERROR;
}

int8_t player_init() {
	t_Error status;

	status = Player_Init(&player_handle);
	ASSERT_TDP_RESULT(status, "player init");

	status = Player_Source_Open(player_handle, &source_handle);
	ASSERT_TDP_RESULT(status, "player open");

	return NO_ERROR;
}

int8_t player_deinit() {
	t_Error status;

	status = Player_Source_Close(player_handle, source_handle);
	ASSERT_TDP_RESULT(status, "player close");

	status = Player_Deinit(player_handle);
	ASSERT_TDP_RESULT(status, "player deinit");

	return NO_ERROR;
}

int8_t demux_init(uint32_t PID, uint32_t tableID, int32_t(*demux_filter_callback)(uint8_t* buffer)) {
    t_Error status;
    status = Demux_Set_Filter(player_handle, PID, tableID, &filter_handle);
	ASSERT_TDP_RESULT(status, "demux set filter");

	status = Demux_Register_Section_Filter_Callback(demux_filter_callback);
	ASSERT_TDP_RESULT(status, "register section filter");
}

int8_t filter_pat() {
	pthread_mutex_lock(&mutex);
}

static int32_t pat_filter_callback(uint8_t* buffer) {
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

	while(n > 0) {
		program_no = (uint16_t) (*(program_section_ptr) << 8)
								+ (*(program_section_ptr + 1));
		
		if(program_no != 0) { 
			pid = (uint16_t) ((*(program_section_ptr + 2) << 8)
							 + (*(program_section_ptr + 3))
							 & 0x1fff);
			printf("Program number %d, PID: %d \n", program_no, pid);
			service_info_array[i].program_no = program_no;
			service_info_array[i].pid = pid;
			i++;
		}
		n -= 4;
		if(n > 0) {
			program_section_ptr += 4;
		}	
	}
	pthread_mutex_lock(&mutex);
	pthread_cond_signal(&condition_pat);
	pthread_mutex_unlock(&mutex);
	printf("Pat table parsed \n");
} 

static int32_t tuner_callback(t_LockStatus status) {
	if(status == STATUS_LOCKED) {
		pthread_mutex_lock(&mutex);
		pthread_cond_signal(&condition);
		pthread_mutex_unlock(&mutex);
		printf("Callback locked \n");
	} else {
		printf("Callback not locked \n");
	}

	return 0;
}
