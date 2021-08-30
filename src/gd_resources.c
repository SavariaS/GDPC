#include "gd_resources.h"
#include "file_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum 
{
    GD_RESOURCE_TYPE_UNKNOWN = 0,

    GD_RESOURCE_TYPE_BITMAP = 1,
    GD_RESOURCE_TYPE_IMAGE = 2,
    GD_RESOURCE_TYPE_TEXTURE = 3,
    GD_RESOURCE_TYPE_TEXTURE3D = 4,
    GD_RESOURCE_TYPE_TEXTURE_ARRAY = 5,
    GD_RESOURCE_TYPE_TEXTURE_ATLAS = 6
};

static gd_file* get_mapped_file(gd_file* file_list, int file_count, FILE* pack);
static int get_resource_type(FILE* pack);

static char* get_string(const char* item, FILE* pack);

bool is_import(const char* path)
{
    path += strlen(path);
    return (strncmp(path - 7, ".import", 7) == 0) ? true : false;
}

int convert_resource(gd_file* file_info, gd_file* file_list, int file_count, FILE* pack, config* cfg)
{
    // Get mapped file
    fseek(pack, file_info->offset, SEEK_SET);
    gd_file* mapped_file = get_mapped_file(file_list, file_count, pack);

    // Get resource type
    fseek(pack, file_info->offset, SEEK_SET);
    int resource_type = get_resource_type(pack);

    // Create path to the extracted file
    file_info->path[strlen(file_info->path) - 7] = '\0'; // remove ".import"
    char* path = generate_path(file_info->path, cfg->destination, strlen(cfg->destination));
    create_path(path);

    // Extract file
    (void)resource_type;
    (void)mapped_file;

    // Clean-up
    file_info->path[strlen(file_info->path)] = '.';
    free(path);

    return 0;
}

static gd_file* get_mapped_file(gd_file* file_list, int file_count, FILE* pack)
{
    // Get the path
    char* path = get_string("path", pack);

    // Find the file in the file list
    gd_file* file = NULL;
    for(int i = 0; i < file_count; ++i)
    {
        if(strcmp(path, file_list[i].path) == 0)
        {
            file = &file_list[i];
            break;
        }
    }

    free(path);

    return file;
}

static int get_resource_type(FILE* pack)
{
    // Get type
    char* str = get_string("type", pack);
    int resource_type = GD_RESOURCE_TYPE_UNKNOWN;

    if(strcmp(str, "StreamTexture") == 0) resource_type = GD_RESOURCE_TYPE_TEXTURE;
    else if(strcmp(str, "Image") == 0)    resource_type = GD_RESOURCE_TYPE_IMAGE;
    else if(strcmp(str, "Texture") == 0)  resource_type = GD_RESOURCE_TYPE_TEXTURE_ATLAS;

    free(str);
    return resource_type;
}

static char* get_string(const char* item, FILE* pack)
{
    size_t len = 0;
    size_t item_length = strlen(item);
    char* buf = malloc(item_length);
    char c = '\0';

    // Find the line that starts with the item
    while(1)
    {
        // Check if the line starts with the item
        fread(buf, 1, item_length, pack);
        if(strncmp(buf, item, item_length) == 0) break;

        // Else, ignore that line
        fseek(pack, -item_length, SEEK_CUR); // Reset cursor to the start of the line
        do
        {
            fread(&c, 1, 1, pack);
        } while (c != '\n');   
    }

    // Get the length of the path
    fseek(pack, 2, SEEK_CUR); // Skip ="
    while(c != '"')
    {
        fread(&c, 1, 1, pack);
        ++len;
    }

    // Store the path
    fseek(pack, -len, SEEK_CUR); // Set cursor back to the start of the path

    char* str = malloc(len);
    if(str == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    fread(str, 1, len - 1, pack);
    str[len - 1] = '\0';

    // Clean-up
    free(buf);

    return str;
}