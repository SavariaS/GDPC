#include "gdpc.h"
#include "gd_resources.h"
#include "file_utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static int read_pack(const char* path, config* cfg);
static int read_file_list(FILE* pack, gd_file* file_list, int file_count, config* cfg);
static int read_files(FILE* pack, gd_file* file_list, int file_count, config* cfg);

int read_packs(config* cfg)
{
    int error = 0;

    // For each pack in the inputs...
    for(size_t i = 0; i < cfg->input_files.size; ++i)
    {
        // Read the pack
        char* file = ((char**)cfg->input_files.data)[i];
        if(read_pack(file, cfg) != 0)
        {
            error = 1;
        }
    }

    return error;
}

/*
 * 1 x 4B  | String | Magic Number (GDPC)
 * 1 x 4B  | Int    | ?
 * 3 x 4B  | Int    | Engine version
 * 1 x 64B | Void   | Reserved
 * 1 x 4B  | Int    | Number of packaged files 
*/
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

    gd_file* list = malloc(file_count * sizeof(gd_file));
    if(list == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    // Print additional information if verbose
    if(cfg->operation_mode == OPERATION_MODE_LIST || cfg->verbose == true)
    {
        printf("\033[4m%s\033[24m\n", path);
        if(cfg->verbose == true)
        {
            printf("Godot v%d.%d.%d\nFile count: %d\n\n", major, minor, revision, file_count);
        }
    }

    // Read the file list
    read_file_list(pack, list, file_count, cfg);

    // Extract the files
    if(cfg->operation_mode == OPERATION_MODE_EXTRACT)
    {
        read_files(pack, list, file_count, cfg);
    }

    // Clean-up
    for(int i = 0; i < file_count; ++i) free(list[i].path);
    free(list);
    fclose(pack);

    return 0;
}

/*
 * 1 x 4B  | Int    | String length
 *         | String | Path
 * 1 x 8B  | Int    | File offset
 * 1 x 8B  | Int    | File size
 * 1 x 16B | ?      | MD5
*/
static int read_file_list(FILE* pack, gd_file* file_list, int file_count, config* cfg)
{
    for(int i = 0; i < file_count; ++i)
    {
        // Get the length of the path
        int len;
        fread(&len, 4, 1, pack);

        file_list[i].path = malloc(len + 1);
        if(file_list[i].path == NULL)
        {
            fprintf(stderr, "malloc(): failed to allocate memory.\n");
            abort();
        }
        fread(file_list[i].path, 1, len, pack);
        file_list[i].path[len] = '\0';

        // Get the real length of the path and resize it if neccessary
        file_list[i].len = strlen(file_list[i].path);
        if(file_list[i].len != len)
        {
            char* new = realloc(file_list[i].path, file_list[i].len + 1);
            if(new == NULL)
            {
                fprintf(stderr, "malloc(): failed to allocate memory.\n");
                abort();
            }

            file_list[i].path = new;
        }

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
        printf("\n");
    }

    return 0;
}

static int read_files(FILE* pack, gd_file* file_list, int file_count, config* cfg)
{
    size_t dest_len = strlen(cfg->destination);

    // For each files in the pack...
    for(int i = 0; i < file_count; ++i)
    {
        // Get file info
        gd_file* file_info = &file_list[i];

        // If the file should be extracted...
        if(is_whitelisted(file_info->path, file_info->len, cfg) && !is_blacklisted(file_info->path, file_info->len, cfg))
        {
            // Extract the file
            char* path = generate_path(file_info->path, cfg->destination, dest_len);
            create_path(path);

            fseek(pack, file_info->offset, SEEK_SET);
            extract_file(path, pack, file_info->size);

            // Clean up
            free(path);
        }

        // Else if it's an .import file and resource files should be converted...
        else if(cfg->convert == true && is_import(file_info->path) == true)
        {
            // Extract resource
            convert_resource(file_info, file_list, file_count, pack, cfg);
        }
    }

    return 0;
}