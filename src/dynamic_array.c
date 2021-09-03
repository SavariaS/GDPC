#include "dynamic_array.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void dynamic_array_reserve(dynamic_array* arr, size_t new_capacity);
static void* at(dynamic_array* arr, size_t index);

void dynamic_array_init(dynamic_array* arr, size_t type_size)
{
    if(arr == NULL) return;

    arr->sizeof_type = type_size;
    arr->data = (char*)malloc(arr->sizeof_type);

    if(arr->data == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    arr->size = 0;
    arr->capacity = 1;
}

void dynamic_array_free(dynamic_array* arr) 
{
    if(arr == NULL) return;
    if(arr->data != NULL)
    {
        free(arr->data);

        arr->data = NULL;
        arr->sizeof_type = 0;
        arr->size = 0;
        arr->capacity = 0;
    }
}

void dynamic_array_push_back(dynamic_array* arr, void* data) 
{
    if(arr == NULL) return;
    if(arr->size + 1 > arr->capacity)
    {
        dynamic_array_reserve(arr, arr->capacity * 2);
    }

    memcpy(at(arr, arr->size), data, arr->sizeof_type);
    arr->size++;
}

void dynamic_array_pop_back(dynamic_array* arr)
{
    if(arr == NULL) return;
    arr->size--;
}

static void dynamic_array_reserve(dynamic_array* arr, size_t new_capacity) 
{
    if(arr == NULL) return;
    if(arr->capacity <= new_capacity)
    {
        void* new_data = realloc(arr->data, new_capacity * arr->sizeof_type);
        if(new_data != NULL)
        {
            arr->data = (char*)new_data;
            arr->capacity = new_capacity;
        }
        else
        {
            fprintf(stderr, "realloc(): failed to re-allocate memory.\n");
            abort();
        }
    }
}

void dynamic_array_shrink(dynamic_array* arr)
{
    dynamic_array_reserve(arr, arr->size);
}

static void* at(dynamic_array* arr, size_t index) 
{ 
    if(arr == NULL || arr->data == NULL) return NULL;
    return (void*)&arr->data[index * arr->sizeof_type]; 
}