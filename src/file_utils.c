#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* generate_path(const char* file, const char* dest, size_t dest_len)
{
    size_t file_len = strlen(file) - 6; // ignore "res://"
    char* path = malloc(dest_len + file_len + 1);
    if(path == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    strcpy(path, dest);
    strcat(path, file + 6);

    return path;
}

int extract_file(const char* dest, FILE* source, int length)
{
    // Open/create extracted file
    FILE* file = fopen(dest, "w");
    if(file == NULL)
    {
        fprintf(stderr, "fopen(): failed to open \"%s\"\n", dest);
        return 1;
    }

    // Copy the contents of the archive to the extracted file
    int character;
    for(int i = 0; i < length; ++i)
    {
        character = fgetc(source);
        fputc(character, file);
    }

    // Clean up
    fclose(file);

    return 0;
}

// Platform-dependant functions
#ifdef __linux__
#include <sys/stat.h>
#include <sys/types.h>

static void create_dir(char* path)
{
    char* separator = strrchr(path, '/');
    if(separator != NULL)
    {
        *separator = '\0';
        create_dir(path);
        *separator = '/';
    }
    mkdir(path, 0777);
}

void create_path(char* path)
{
    char* separator = strrchr(path, '/');
    if(separator != NULL)
    {
        *separator = '\0';
        create_dir(path);
        *separator = '/';
    }
}
#endif