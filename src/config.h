#ifndef TOOL_GDPC_CONFIG_H
#define TOOL_GDPC_CONFIG_H

#include "dynamic_array.h"
#include <stddef.h>
#include <stdint.h>
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
    bool convert;

    int32_t version_major;
    int32_t version_minor;
    int32_t version_revision;

    int operation_mode;

    dynamic_array whitelist;
    dynamic_array blacklist;
    dynamic_array input_files;
    char* destination;
} config;

bool parse_command_line_arguments(int argc, char** argv, config* cfg);
void free_config(config* cfg);

#endif