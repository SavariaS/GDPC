#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool filter_path(dynamic_array* filters, char* path, int len);

char* generate_path(const char* file, const char* dest, size_t dest_len)
{
    size_t file_len = strlen(file) - 6; // Ignore "res://"
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

bool is_whitelisted(char* file, int len, config* cfg) 
{ 
    if(cfg->whitelist.size == 0)
    {
        return true;
    }
    else
    {
        return filter_path(&cfg->whitelist, file, len); 
    }
}

bool is_blacklisted(char* file, int len, config* cfg) 
{ 
    if(cfg->blacklist.size == 0)
    {
        return false;
    }
    else
    {
        return filter_path(&cfg->blacklist, file, len); 
    }
}

static bool filter_path(dynamic_array* filters, char* path, int len)
{
    path += 6; // Ignore "res://"
    len -= 6 - 1;

    // For each filter 
    for(size_t i = 0; i < filters->size; ++i)
    {
        filter* fil = (filter*)&filters->data[sizeof(filter) * i];

        // If there is no wildcard character...
        if(fil->wildcard == NULL)
        {
            // Compare both strings
            if(strcmp(path, fil->data) == 0)
            {
                return true;
            }
        }
        else
        {
            // Get the length of the string after the wildcard charater
            int end_len = fil->end - (fil->wildcard + 1);

            // Compare the strings on either side of the wildcard character
            if(strncmp(path, fil->data, fil->wildcard - fil->data) == 0 &&
               strncmp(path + len - end_len, fil->wildcard + 1, end_len) == 0)
            {
                return true;
            }
        }
    }

    return false;
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