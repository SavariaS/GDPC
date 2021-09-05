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

static void write_file_list(FILE* pack, dynamic_array* files, dynamic_array* files_names, dynamic_array* files_names_lengths, config* cfg);
static void write_file_list_item(FILE* pack, dynamic_array* files, dynamic_array* files_names, dynamic_array* files_names_lengths, char* path, int32_t path_len, char* file_path, int32_t file_path_len, int64_t offset, int64_t size);
static void write_files(FILE* pack, dynamic_array* files, dynamic_array* files_names, dynamic_array* files_names_lengths, int64_t* list_offset, int64_t* file_offset, config* cfg);

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

/* File header
 * 1 x 4B  | String | Magic Number (0x47445043)
 * 1 x 4B  | Int    | ?
 * 3 x 4B  | Int    | Engine version
 * 1 x 64B | Void   | Reserved
 * 1 x 4B  | Int    | Number of packaged files 
*/
static int read_pack(const char* path, 
                     config* cfg)
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

/* File list item
 * 1 x 4B  | Int    | String length
 *         | String | Path
 * 1 x 8B  | Int    | File offset
 * 1 x 8B  | Int    | File size
 * 1 x 16B | ?      | MD5
*/
static int read_file_list(FILE* pack, 
                          gd_file* file_list, 
                          int file_count, 
                          config* cfg)
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
    }

    return 0;
}

static int read_files(FILE* pack, 
                      gd_file* file_list, 
                      int file_count, 
                      config* cfg)
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

/* File header
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
        // If creating a file...
        if(cfg->operation_mode == OPERATION_MODE_CREATE)
        {
            printf("Creating \033[4m%s\033[24m (v%d.%d.%d)\n", cfg->destination, cfg->version_major, cfg->version_minor, cfg->version_revision);
        }
        // If updating a file...
        else
        {
            char** input_files = (char**)cfg->input_files.data;
            printf("Updating \033[4m%s\033[24m\n", input_files[cfg->input_files.size - 1]);
        }
    }

    // Write file header
    if(cfg->operation_mode == OPERATION_MODE_CREATE)
    {
        // Create file header from configuration
        char magic[4] = {'G', 'D', 'P', 'C'}; // Magic number
        fwrite(magic, 1, 4, pack);

        int32_t zero = 0;
        fwrite(&zero, 1, 4, pack); // ?

        fwrite(&cfg->version_major, 4, 1, pack); // Engine version
        fwrite(&cfg->version_minor, 4, 1, pack);
        fwrite(&cfg->version_revision, 4, 1, pack);

        for(int i = 0; i < 16; ++i) fwrite(&zero, 4, 1, pack); // Reserved space

        fwrite(&zero, 4, 1, pack); // Number of files
    }
    else
    {
        // Copy file header from package
        char buf[88];
        FILE* original = fopen(((char**)cfg->input_files.data)[cfg->input_files.size - 1], "r");
        if(original == NULL)
        {
            printf("gdpc: Failed to open file \"%s\"\n", ((char**)cfg->input_files.data)[cfg->input_files.size - 1]);
            return 1;
        }

        fread(buf, 1, 88, original);
        fwrite(buf, 1, 88, pack);

        fclose(original);
    }

    // Write file list
    dynamic_array files;
    dynamic_array files_names;
    dynamic_array files_names_lengths;
    dynamic_array_init(&files, sizeof(gd_file));
    dynamic_array_init(&files_names, sizeof(char*));
    dynamic_array_init(&files_names_lengths, sizeof(int32_t));

    write_file_list(pack, &files, &files_names, &files_names_lengths, cfg);

    // Write files
    int64_t list_offset = 88;
    int64_t file_offset = ftell(pack);

    write_files(pack, &files, &files_names, &files_names_lengths, &list_offset, &file_offset, cfg);

    // Clean up
    char** arr = (char**)files_names.data;
    for(size_t i = 0; i < files_names.size; ++i)
    {
        free(arr[i]);
    }

    dynamic_array_free(&files);
    dynamic_array_free(&files_names);
    dynamic_array_free(&files_names_lengths);

    fclose(pack);

    // If updating a packge
    if(cfg->operation_mode == OPERATION_MODE_UPDATE)
    {
        char* old_package = ((char**)cfg->input_files.data)[cfg->input_files.size - 1];

        // Overwrite old package
        remove(old_package);
        rename(cfg->destination, old_package);
    }

    return 0;
}

/* File list item
 * 1 x 4B  | Int    | String length
 *         | String | Path
 * 1 x 8B  | Int    | File offset
 * 1 x 8B  | Int    | File size
 * 1 x 16B | ?      | MD5
*/
static void write_file_list(FILE* pack, 
                            dynamic_array* files, 
                            dynamic_array* files_names, 
                            dynamic_array* files_names_lengths, 
                            config* cfg)
{
    char** file_list = (char**)cfg->input_files.data;

    // For each input file
    for(size_t i = 0; i < cfg->input_files.size; ++i)
    {
        char* file = file_list[i];

        // If file isn't a regular file, ignore it
        if(is_regular_file(file) == false)
        {
            continue;
        }
        // If the file isn't a .pck, add the file to the list
        else if(is_pck(file) == false)
        {
            // Generate path
            int32_t len = strlen(file) + 6;
            char* path = calloc(len + 1, 1);
            if(path == NULL)
            {
                fprintf(stderr, "calloc(): failed to allocate memory.\n");
                abort();
            }

            char res[7] = "res://";
            strcat(path, res);
            strcat(path, file);

            // Write item
            write_file_list_item(pack, files, files_names, files_names_lengths, path, len, file, len - 6, 0, -1);
        }
        // If the file is a .pck, add each packaged file to the list
        else
        {
            // Open file
            FILE* package = fopen(file, "rb");
            if(package == NULL)
            {
                printf("gdpc: Failed to open file \"%s\"\n", cfg->destination);
                return;
            }

            // Get the number of files in the package
            fseek(package, 84, SEEK_SET);

            int32_t file_count;
            fread(&file_count, 4, 1, package);

            // For each file in the package
            for(int32_t j = 0; j < file_count; ++j)
            {
                // Get the length of the string
                int32_t str_len;
                fread(&str_len, 4, 1, package);

                // Get the path
                char* path = malloc(str_len + 1);
                fread(path, 1, str_len, package);
                path[str_len] = '\0';

                // Get the offset and size
                int64_t offset, size;
                fread(&offset, 8, 1, package);
                fread(&size, 8, 1, package);

                // Skip MD5
                fseek(package, 16, SEEK_CUR);

                // Write item
                write_file_list_item(pack, files, files_names, files_names_lengths, path, str_len, file, strlen(file), offset, size);
            }
        }
    }

    // Store number of files
    uint32_t file_count = files->size;

    fseek(pack, 84, SEEK_SET);
    fwrite(&file_count, 4, 1, pack);
    fseek(pack, 0, SEEK_END);

    if(cfg->verbose == true)
    {
        printf("Storing %d files:\n", (int)file_count);
    }
}

static void write_file_list_item(FILE* pack, 
                                 dynamic_array* files, 
                                 dynamic_array* files_names, 
                                 dynamic_array* files_names_lengths, 
                                 char* path, 
                                 int32_t path_len, 
                                 char* file_path, 
                                 int32_t file_path_len, 
                                 int64_t offset, 
                                 int64_t size
                                 )
{
    // Check if item is already present
    char** items = (char**)files_names->data;
    for(size_t i = 0; i < files_names->size; ++i)
    {
        if(strcmp(items[i], path) == 0)
        {
            // Ignore this item
            free(path);
            return;
        }
    }
    // Write item
    char zero[32] = { 0 };

    fwrite(&path_len, 4, 1, pack); // Length
    fwrite(path, 1, path_len, pack); // Path
    fwrite(zero, 1, 32, pack); // Set offset, size and MD5 to 0

    // Add file to list of files to package
    gd_file gdf = { file_path, file_path_len, offset, size};
    dynamic_array_push_back(files, &gdf);

    // Store path
    dynamic_array_push_back(files_names, &path);
    dynamic_array_push_back(files_names_lengths, &path_len);
}

static void write_files(FILE* pack, 
                        dynamic_array* files, 
                        dynamic_array* files_names, 
                        dynamic_array* files_names_lengths, 
                        int64_t* list_offset, 
                        int64_t* file_offset, 
                        config* cfg
                        )
{
    gd_file* file_list = (gd_file*)files->data;
    char** name_list = (char**)files_names->data;
    int32_t* length_list = (int32_t*)files_names_lengths->data;

    // For each file to be added to the package...
    for(size_t i = 0; i < files->size; ++i)
    {
        gd_file* gdf = &file_list[i];

        // Open file
        FILE* file = fopen(gdf->path, "rb");
        if(file == NULL)
        {
            printf("gdpc: Failed to read from file \"%s\"\n", gdf->path);
            *list_offset += length_list[i] + 36;

            continue;
        }

        // Print additional informative message if --verbose
        if(cfg->verbose == true)
        {
            printf("Packaging \"%s\"\n", name_list[i]);
        }

        // Copy file
        uint64_t size = 0;
        fseek(file, gdf->offset, SEEK_SET);

        if(gdf->size == -1)
        {
            int c = fgetc(file);
            while(c != EOF)
            {
                ++size;
                fputc(c, pack);
                c = fgetc(file);
            }
        }
        else
        {
            size = gdf->size;
            int c;

            for(int32_t i = 0; i < gdf->size; ++i)
            {
                c = fgetc(file);
                fputc(c, pack);
            }
        }

        fclose(file);

        // Store offset and size
        fseek(pack, *list_offset + length_list[i] + 4, SEEK_SET); // Go to file list

        fwrite(file_offset, 8, 1, pack); // Store offset
        fwrite(&size, 8, 1, pack); // Store size

        // Adjust values
        *list_offset += length_list[i] + 36;
        *file_offset += size;
        fseek(pack, 0, SEEK_END);
    }
}