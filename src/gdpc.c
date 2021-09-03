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

static int32_t get_file_count(config* cfg);
static void write_file_list(FILE* pack, char** file_list, int32_t* length_list, config* cfg);
//static int write_file_list_from_pack();
static void write_files(FILE* pack, char** file_list, int32_t* length_list, int64_t* list_offset, int64_t* file_offset, config* cfg);
//static int write_files_from_pack();

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
 * 1 x 4B  | String | Magic Number (0x47445043)
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
    int32_t file_count = 0;
    fread(&file_count, 4, 1, pack);

    gd_file* list = malloc(file_count * sizeof(gd_file));
    if(list == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    // Print additional information if verbose
    if(cfg->verbose == true)
    {
        printf("\033[4m%s\033[24m (v%d.%d.%d) (%d files found)\n", path, major, minor, revision, file_count);
    }
    else if(cfg->operation_mode == OPERATION_MODE_LIST)
    {
        printf("\033[4m%s\033[24m\n", path);
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
        //printf("\n");
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
            if(cfg->verbose == true)
            {
                printf("Extracting \"%s\" (%ldB)\n", file_info->path, file_info->size);
            }

            // Extract the file
            char* path = generate_path(file_info->path, cfg->destination, dest_len);
            create_path(path);

            fseek(pack, file_info->offset, SEEK_SET);
            int success = extract_file(path, pack, file_info->size);

            // Clean up
            free(path);

            if(success != 0)
            {
                continue;
            }
        }
        else if(cfg->verbose == true)
        {
            printf("Ignoring \"%s\"\n", file_info->path);
        }

        // Else if it's an .import file and resource files should be converted...
        if(cfg->convert == true && is_import(file_info->path) == true)
        {
            // Extract resource
            convert_resource(file_info, file_list, file_count, pack, cfg);
        }
    }

    return 0;
}

/*
 * 1 x 4B  | String | Magic Number (0x47445043)
 * 1 x 4B  | Int    | ?
 * 3 x 4B  | Int    | Engine version
 * 1 x 64B | Void   | Reserved
 * 1 x 4B  | Int    | Number of packaged files 
*/
int create_pack(config* cfg)
{
    // Create file
    create_path(cfg->destination);
    FILE* pack = fopen(cfg->destination, "wb");
    if(pack == NULL)
    {
        printf("gdpc: Failed to create file \"%s\"\n", cfg->destination);
        return 1;
    }

    if(cfg->verbose == true)
    {
        printf("Creating \033[4m%s\033[24m (v%d.%d.%d)\n", cfg->destination, cfg->version_major, cfg->version_minor, cfg->version_revision);
    }

    // Write file header
    char magic[4] = {'G', 'D', 'P', 'C'}; // Magic number
    fwrite(magic, 1, 4, pack);

    int32_t zero = 0;
    fwrite(&zero, 1, 4, pack); // ?

    fwrite(&cfg->version_major, 4, 1, pack); // Engine version
    fwrite(&cfg->version_minor, 4, 1, pack);
    fwrite(&cfg->version_revision, 4, 1, pack);

    for(int i = 0; i < 16; ++i) fwrite(&zero, 4, 1, pack); // Reserved space

    int32_t fileCount = get_file_count(cfg);
    fwrite(&fileCount, 4, 1, pack); // Number of files

    // Write file list
    char** file_list = (char**)cfg->input_files.data;
    int32_t* length_list = malloc(sizeof(int32_t) * cfg->input_files.size);

    if(cfg->verbose == true)
    {
        printf("Storing %d files:\n", (int)fileCount);
    }

    write_file_list(pack, file_list, length_list, cfg);

    // Write files
    int64_t list_offset = 80;
    int64_t file_offset = ftell(pack);

    write_files(pack, file_list, length_list, &list_offset, &file_offset, cfg);

    // Clean up
    free(length_list);
    fclose(pack);

    return 0;
}

static int32_t get_file_count(config* cfg)
{
    char** file_list = (char**)cfg->input_files.data;
    int32_t count = 0;

    for(size_t i = 0; i < cfg->input_files.size; ++i)
    {
        if(is_file(file_list[i]))
        {
            ++count;
        }
    }

    return count;
}

/*
 * 1 x 4B  | Int    | String length
 *         | String | Path
 * 1 x 8B  | Int    | File offset
 * 1 x 8B  | Int    | File size
 * 1 x 16B | ?      | MD5
*/
static void write_file_list(FILE* pack, char** file_list, int32_t* length_list, config* cfg)
{
    for(size_t i = 0; i < cfg->input_files.size; ++i)
    {
        // Get file
        char* original_path = file_list[i];

        if(is_file(original_path) == false)
        {
            continue;
        }

        // Generate path
        length_list[i] = strlen(original_path) + 6;
        char* path = calloc(length_list[i] + 1, 1);

        char res[7] = "res://";
        strcat(path, res);
        strcat(path, original_path);

        // Write file item
        char zero[32] = { 0 };

        fwrite(&length_list[i], 4, 1, pack); // Length
        fwrite(path, 1, length_list[i], pack); // Path
        fwrite(zero, 1, 32, pack); // Set offset, size and MD5 to 0

        free(path);
    }
}

static void write_files(FILE* pack, char** file_list, int32_t* length_list, int64_t* list_offset, int64_t* file_offset, config* cfg)
{
    for(size_t i = 0; i < cfg->input_files.size; ++i)
    {
        // Get file
        char* original_file = file_list[i];

        if(is_file(original_file) == false)
        {
            continue;
        }

        // Copy file
        FILE* file = fopen(original_file, "rb");
        if(file == NULL)
        {
            printf("gdpc: Failed to read from file \"%s\"\n", cfg->destination);
            *list_offset += length_list[i] + 36;

            continue;
        }

        if(cfg->verbose == true)
        {
            printf("Packaging \"%s\"\n", original_file);
        }

        uint64_t size = 0;
        int c = fgetc(file);
        while(c != EOF)
        {
            ++size;
            fputc(c, pack);
            c = fgetc(file);
        }
        fclose(file);

        // Store offset and size
        fseek(pack, *list_offset + length_list[i] + 12, SEEK_SET); // Go to file list

        fwrite(file_offset, 8, 1, pack); // Store offset
        fwrite(&size, 8, 1, pack); // Store size

        // Adjust values
        *list_offset += length_list[i] + 36;
        *file_offset += size;
        fseek(pack, 0, SEEK_END);
    }
}