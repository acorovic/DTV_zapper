#include "init_parser.h"

static const char *valid_options[] = {"frequency", "bandwidth", "modulation",
                                     "video_pid", "audio_pid", "video_type",
                                     "audio_type", "program_number"};
static const char delimiter[] = "=";

static parser_state_t parser_state;

/* Helper functions */
static void add_value(char* value, int8_t option);
static int8_t check_option(char* option);

int8_t init_file_parse(const char* file_path)
{
    FILE* file_ptr;
    char* line = NULL;
    char* option = NULL;
    char* value = NULL;
    size_t len = 0;
    int8_t read = 0;
    int8_t check_status;

    file_ptr = fopen(file_path, "r");
    if (file_ptr == 0)
    {
        printf("Can't open file %s \n", file_path);
        return -1;
    }

    while ((read = getline(&line, &len, file_ptr)) != -1)
    {
        option = strtok(line, delimiter);
        check_status = check_option(option);
        if (check_status == -1)
        {
            printf("Unknown option \"%s\" in init file! \n", option);
            return -1;
        }
        value = strtok(NULL, delimiter);
        add_value(value, check_status);
    }
    fclose(file_ptr);
    free(line);

    return 0;
}

int32_t parser_get_frequency() {
    int32_t ret_val = -1;
    char* ptr;

    if (parser_state.freq.is_read == 1)
    {
        ret_val = strtol(parser_state.freq.value, &ptr, 10);
        return ret_val;
    } else
    {
        return -1;
    }
}

int32_t parser_get_band() {
    int32_t ret_val = -1;
    char* ptr;

    if (parser_state.band.is_read == 1)
    {
        ret_val = strtol(parser_state.band.value, &ptr, 10);
        return ret_val;
    } else
    {
        return -1;
    }
}

char* parser_get_modulation() {
    if (parser_state.mod.is_read == 1)
    {
        return parser_state.mod.value;
    } else
    {
        return NULL;
    }
}

int32_t parser_get_video_pid() {
    int32_t ret_val = -1;
    char* ptr;

    if (parser_state.v_pid.is_read == 1)
    {
        ret_val = strtol(parser_state.v_pid.value, &ptr, 10);
        return ret_val;
    } else
    {
        return -1;
    }
}

int32_t parser_get_audio_pid() {
    int32_t ret_val = -1;
    char* ptr;

    if (parser_state.a_pid.is_read == 1)
    {
        ret_val = strtol(parser_state.a_pid.value, &ptr, 10);
        return ret_val;
    } else
    {
        return -1;
    }
}

char* parser_get_video_type() {
    if (parser_state.v_type.is_read == 1)
    {
        return parser_state.v_type.value;
    } else
    {
        return NULL;
    }
}

char* parser_get_audio_type() {
    if (parser_state.a_type.is_read == 1)
    {
        return parser_state.a_type.value;
    } else
    {
        return NULL;
    }
}

int32_t parser_get_program_number() {
    int32_t ret_val = -1;
    char* ptr;

    if (parser_state.p_no.is_read == 1)
    {
        ret_val = strtol(parser_state.p_no.value, &ptr, 10);
        return ret_val;
    } else
    {
        return -1;
    }
}

static int8_t check_option(char* option) {
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        if (strcmp(option, valid_options[i]) == 0)
        {
            return i;
        }
    }
    return -1;
}

static void add_value(char* value, int8_t option) {
    switch (option)
    {
    case 0:
        parser_state.freq.is_read = 1;
        strcpy(parser_state.freq.value, value);
        break;
    case 1:
        parser_state.band.is_read = 1;
        strcpy(parser_state.band.value, value);
        break;
    case 2:
        parser_state.mod.is_read = 1;
        strcpy(parser_state.mod.value, value);
        break;
    case 3:
        parser_state.v_pid.is_read = 1;
        strcpy(parser_state.v_pid.value, value);
        break;
    case 4:
        parser_state.a_pid.is_read = 1;
        strcpy(parser_state.a_pid.value, value);
        break;
    case 5:
        parser_state.v_type.is_read = 1;
        strcpy(parser_state.v_type.value, value);
        break;
    case 6:
        parser_state.a_type.is_read = 1;
        strcpy(parser_state.a_type.value, value);
        break;
    case 7:
        parser_state.p_no.is_read = 1;
        strcpy(parser_state.p_no.value, value);
        break;
    }
}
