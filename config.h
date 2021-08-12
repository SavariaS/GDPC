#ifndef TOOL_GDPC_CONFIG_H
#define TOOL_GDPC_CONFIG_H

#include <stdbool.h>

enum
{
    OPERATION_MODE_UNSPECIFIED = 0,
    OPERATION_MODE_NONE = 1,
    OPERATION_MODE_LIST = 2,
    OPERATION_MODE_EXTRACT = 3,
    OPERATION_MODE_CREATE = 4,
    OPERATION_MODE_UPDATE = 5
};

typedef struct
{
    bool verbose;

    int operation_mode;

    int input_count;
    char** input_files;
    char* destination;
} config;

bool parse_command_line_arguments(int argc, char** argv, config* cfg);

#endif