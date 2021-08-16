#ifndef TOOL_GDPC_RESOURCES_H
#define TOOL_GDPC_RESOURCES_H

#include "config.h"
#include <stdio.h>
#include <stdint.h>

typedef struct
{
    char* path;
    int64_t offset;
    int64_t size;
} gd_file;

int is_godot_extension(const char* path);
int is_import(const char* path);

int extract_resource_from_import(gd_file* file_info, gd_file* file_list, int file_count, FILE* pack, config* cfg);

#endif