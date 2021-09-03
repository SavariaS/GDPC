#ifndef TOOL_GDPC_DYNAMIC_ARRAY_H
#define TOOL_GDPC_DYNAMIC_ARRAY_H

#include <stddef.h>

typedef struct 
{
    char* data;
    size_t sizeof_type;

    size_t size;
    size_t capacity;
} dynamic_array;

void dynamic_array_init(dynamic_array* arr, size_t type_size);
void dynamic_array_free(dynamic_array* arr);

void dynamic_array_push_back(dynamic_array* arr, void* data);
void dynamic_array_pop_back(dynamic_array* arr);
void dynamic_array_shrink(dynamic_array* arr);

#endif