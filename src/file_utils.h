#ifndef TOOL_GDPC_UTILS_H
#define TOOL_GDPC_UTILS_H

#include <stdio.h>

char* generate_path(const char* file, const char* dest, size_t dest_len);
int extract_file(const char* dest, FILE* source, int length);

void create_path(char* path); // Platform-dependant

#endif