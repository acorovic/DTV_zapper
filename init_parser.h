#ifndef INIT_PARSER_H
#define INIT_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct init_options {
    char value[11];
    uint8_t is_read;
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

int8_t init_file_parse(const char* file_path);
int32_t parser_get_frequency();
int32_t parser_get_band();
char* parser_get_modulation();
int32_t parser_get_video_pid();
int32_t parser_get_audio_pid();
char* parser_get_video_type();
char* parser_get_audio_type();
int32_t parser_get_program_number();

#endif
