#ifndef TOOL_GDPC_RESOURCES_H
#define TOOL_GDPC_RESOURCES_H

#include "config.h"
#include "file_utils.h"
#include <stdio.h>

bool is_import(const char* path);
bool is_pck(const char* path);

int convert_resource(gd_file* file_info, gd_file* file_list, int file_count, FILE* pack, config* cfg);

#endif