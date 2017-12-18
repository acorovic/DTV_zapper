#ifndef INIT_PARSER_H
#define INIT_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "common.h"

#define MAX_VALUE_LENGTH 30     /**< Max number of characters read for value from config file */

typedef struct init_options {
    char value[MAX_VALUE_LENGTH];   /**< Value which is read from init file */
    uint8_t is_read;                /**< Flag, 0 if not read, else 1 */
} init_options_t;

typedef struct parser_s {
    init_options_t freq;
    init_options_t band;
    init_options_t mod;
    init_options_t v_pid;
    init_options_t a_pid;
    init_options_t v_type;
    init_options_t a_type;
    init_options_t p_no;
} parser_state_t;

/**
 * @brief Function which parses init file and stores values for options
 *
 * @param file_path Path to file which contains initial configuration
 *
 * @return ERR or NO_ERR
 */
int8_t init_file_parse(const char* file_path);

/**
 * @brief Get value for frequency which is read from init file
 *
 * @return -1 if frequency is not read, else 0
 */
int32_t parser_get_frequency();

/**
 * @brief Get value for bandwidth which is read from init file
 *
 * @return -1 if bandwidth is not read, else 0
 */
int32_t parser_get_bandwidth();

/**
 * @brief Get value for modulation which is read from init file
 *
 * @return NULL if modulation is not read, else string which is read
 */
char* parser_get_modulation();

/**
 * @brief Get value for video_pid which is read from init file
 *
 * @return -1 if video_pid is not read, else 0
 */
int32_t parser_get_video_pid();

/**
 * @brief Get value for audio_pid which is read from init file
 *
 * @return -1 if audio_pid is not read, else 0
 */
int32_t parser_get_audio_pid();

/**
 * @brief Get value for video_type which is read from init file
 *
 * @return NULL if video_type is not read, else string which is read
 */
char* parser_get_video_type();

/**
 * @brief Get value for audio_type which is read from init file
 *
 * @return NULL if audio_type is not read, else string which is read
 */
char* parser_get_audio_type();

/**
 * @brief Get value for program_number which is read from init file
 *
 * @return -1 if audio_pid is not read, else 0
 */
int32_t parser_get_program_number();

#endif
