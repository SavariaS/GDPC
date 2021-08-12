#include "gdpc.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Platform-dependant includes
#ifdef __linux__
#include <sys/stat.h>
#include <sys/types.h>
#endif

typedef struct
{
    char* path;
    int64_t offset;
    int64_t size;
} gdpc_file;

static int read_pack(const char* path, config* cfg);
static int read_file_list(FILE* pack, gdpc_file* file_list, int file_count, config* cfg);
static int extract_files(FILE* pack, gdpc_file* file_list, int file_count, config* cfg);

static void create_dir(char* path);

int read_packs(config* cfg)
{
    int error = 0;

    // For each pack in the inputs...
    for(int i = 0; i < cfg->input_count; ++i)
    {
        // Read the pack
        if(read_pack(cfg->input_files[i], cfg) != 0)
        {
            error = 1;
        }
    }

    return error;
}

static int read_pack(const char* path, config* cfg)
{
    // Open file
    FILE* pack = fopen(path, "r");
    if(pack == NULL)
    {
        printf("gdpc: Failed to open file \"%s\"\n", path);
        return 1;
    }

    // Read magic number
    char magic[4];
    fread(magic, 1, 4, pack);
    if(strncmp(magic, "GDPC", 4) != 0)
    {
        printf("gdpc: File is not a .pck file \"%s\"\n", path);
        return 1;
    }

    // Version
    int32_t wtf, major, minor, revision;
    fread(&wtf, 4, 1, pack);
    fread(&major, 4, 1, pack);
    fread(&minor, 4, 1, pack);
    fread(&revision, 4, 1, pack);

    // Skip reserved space
    fseek(pack, 64, SEEK_CUR);

    // Number of files
    int file_count = 0;
    fread(&file_count, 4, 1, pack);

    gdpc_file* list = malloc(file_count * sizeof(gdpc_file));
    if(list == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    // Print additional information if verbose
    if(cfg->operation_mode == OPERATION_MODE_LIST || cfg->verbose == true)
    {
        printf("%s\n", path);
        if(cfg->verbose == true)
        {
            printf("Godot v%d.%d.%d\nFile count: %d\n", major, minor, revision, file_count);
        }
    }

    // Read the file list
    read_file_list(pack, list, file_count, cfg);

    if(cfg->operation_mode == OPERATION_MODE_EXTRACT)
    {
        extract_files(pack, list, file_count, cfg);
    }

    free(list);
    fclose(pack);

    return 0;
}

static int read_file_list(FILE* pack, gdpc_file* file_list, int file_count, config* cfg)
{
    for(int i = 0; i < file_count; ++i)
    {
        // Get the length of the path
        int len = 0;
        fread(&len, 4, 1, pack);

        file_list[i].path = malloc(len + 1);
        if(file_list[i].path == NULL)
        {
            fprintf(stderr, "malloc(): failed to allocate memory.\n");
            abort();
        }
        fread(file_list[i].path, 1, len, pack);
        file_list[i].path[len] = '\0';

        // Get the offset
        fread(&file_list[i].offset, 8, 1, pack);
        // Get the size
        fread(&file_list[i].size, 8, 1, pack);

        // Skip MD5
        fseek(pack, 16, SEEK_CUR);
    }

    if(cfg->operation_mode == OPERATION_MODE_LIST)
    {
        for(int i = 0; i < file_count; ++i)
        {
            printf("%s\n", file_list[i].path);
        }
        printf("-----\n");
    }

    return 0;
}

static int extract_files(FILE* pack, gdpc_file* file_list, int file_count, config* cfg)
{
    size_t dest_len = strlen(cfg->destination);

    for(int i = 0; i < file_count; ++i)
    {
        // Get the file info
        gdpc_file* file_info = &file_list[i];
        size_t path_len = strlen(file_info->path) - 6;

        // Create the path to the extracted file
        char* path = malloc(dest_len + path_len + 1);
        if(path == NULL)
        {
            fprintf(stderr, "malloc(): failed to allocate memory.\n");
            abort();
        }

        strcpy(path, cfg->destination);
        strcat(path, file_info->path + 6);

        // Create all subdirectories (if neccessary)
        #ifdef __linux__
        char* separator = strrchr(path, '/');
        if(separator != NULL)
        {
            *separator = '\0';
            create_dir(path);
            *separator = '/';
        }
        #endif

        // Open/create extracted file
        FILE* dest = fopen(path, "w");
        if(dest == NULL)
        {
            free(path);
            continue;
        }

        // Copy the contents of the archive to the extracted file
        fseek(pack, file_info->offset, SEEK_SET);

        int data;
        for(int j = 0; j < file_info->size; ++j)
        {
            data = fgetc(pack);
            fputc(data, dest);
        }

        fclose(dest);
        free(path);
    }

    return 0;
}

#ifdef __linux__
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
#endif