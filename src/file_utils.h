#ifndef TOOL_GDPC_UTILS_H
#define TOOL_GDPC_UTILS_H

#include <stdio.h>
#include <stdint.h>
#include "config.h"

typedef struct
{
    char* data;
    char* wildcard;
    char* end;
} filter;

typedef struct
{
    char* path;
    int len;
    
    int64_t offset;
    int64_t size;
} gd_file;

char* generate_path(const char* file, const char* dest, size_t dest_len);
bool is_whitelisted(char* file, int len, config* cfg);
bool is_blacklisted(char* file, int len, config* cfg);

void create_path(char* path); // Platform-dependant
int extract_file(const char* dest, FILE* source, int length);

#endif