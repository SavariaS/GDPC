#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "file_utils.h"

static int parse_long_option(char* arg, config* cfg);
static int parse_short_options(char* arg, config* cfg);
static int parse_value(char* arg, config* cfg);
static int parse_paths(char* arg, config* cfg);

static int add_filter(dynamic_array* arr, char* arg);

static void print_help_message();

bool parse_command_line_arguments(int argc, char** argv, config* cfg)
{
    // Default initialize the configuration
    cfg->verbose = false;
    cfg->convert = false;
    cfg->version_major = 0;
    cfg->version_minor = 0;
    cfg->version_revision = 0;
    cfg->operation_mode = OPERATION_MODE_UNSPECIFIED;
    cfg->destination = NULL;

    dynamic_array_init(&cfg->whitelist, sizeof(filter));
    dynamic_array_init(&cfg->blacklist, sizeof(filter));
    dynamic_array_init(&cfg->input_files, sizeof(char*));

    // For each argument, except the first one (the executable call)
    for(int i = 1; i < argc; ++i)
    {
        // If the argument is a long option...
        if(argv[i][0] == '-' && argv[i][1] == '-')
        {
            if(parse_long_option(argv[i], cfg) != 0) return 1;
        }
        // Else, if it's a value...
        else if(argv[i][0] == '-' && argv[i][2] == '=')
        {
            if(parse_value(argv[i], cfg) != 0) return 1;
        }
        // Else, if it's a short option...
        else if(argv[i][0] == '-')
        {
            if(parse_short_options(argv[i], cfg) != 0) return 1;
        }
        // Else, parse path
        else
        {
            parse_paths(argv[i], cfg);
        }
    }

    // Error checking
    if(cfg->operation_mode == OPERATION_MODE_UNSPECIFIED)
    {
        printf("gdpc: You must specify the operation mode.\nTry 'gdpc --help' for more information.\n");
        return 1;
    }
    if(cfg->input_files.size < 1 && cfg->operation_mode == OPERATION_MODE_LIST)
    {
        printf("gdpc: You must provide files to list.\nTry 'gdpc --help' for more information.\n");
        return 1;
    }
    if(cfg->input_files.size < 2 && cfg->operation_mode != OPERATION_MODE_LIST)
    {
        printf("gdpc: You must provide file(s) to extract/package as well as a destination.\nTry 'gdpc --help' for more information.\n");
        return 1;
    }
    if(cfg->operation_mode == OPERATION_MODE_CREATE && cfg->version_major == 0)
    {
        printf("gdpc: You must specify the engine version when creating packages.\nTry 'gdpc --help' for more information.\n");
        return 1;
    }

    // Set the last input file as the destination when creating or extracting packages
    if(cfg->operation_mode == OPERATION_MODE_CREATE || cfg->operation_mode == OPERATION_MODE_EXTRACT)
    {
        cfg->destination = ((char**)cfg->input_files.data)[cfg->input_files.size - 1]; // Set last file as the destination
        dynamic_array_pop_back(&cfg->input_files); // Remove it from the list of input files

        // If destination is not a folder, add '/'
        size_t len = strlen(cfg->destination);
        if(cfg->operation_mode == OPERATION_MODE_EXTRACT && cfg->destination[len - 1] != '/')
        {
            char* new_path = malloc(len + 2);
            memcpy(new_path, cfg->destination, len);

            new_path[len] = '/';
            new_path[len + 1] = '\0';

            free(cfg->destination);
            cfg->destination = new_path;
        }
    }

    // Set the last input file as the target pack when updating packages
    if(cfg->operation_mode == OPERATION_MODE_UPDATE)
    {
        char* last_file = ((char**)cfg->input_files.data)[cfg->input_files.size - 1];
        size_t len = strlen(last_file) + 7; // Length of the last file + ".update"
        cfg->destination = calloc(len + 1, 1);
        if(cfg->destination == NULL)
        {
            fprintf(stderr, "calloc(): failed to allocate memory.\n");
            abort();
        }

        strcat(cfg->destination, last_file);
        strcat(cfg->destination, ".update");
    }

    // Clean-up
    dynamic_array_shrink(&cfg->whitelist);
    dynamic_array_shrink(&cfg->blacklist);
    dynamic_array_shrink(&cfg->input_files);

    return 0;
}

static int parse_long_option(char* arg, config* cfg)
{
    if(strcmp(arg, "--list") == 0) cfg->operation_mode = OPERATION_MODE_LIST;
    else if(strcmp(arg, "--extract") == 0) cfg->operation_mode = OPERATION_MODE_EXTRACT;
    else if(strcmp(arg, "--create") == 0) cfg->operation_mode = OPERATION_MODE_CREATE;
    else if(strcmp(arg, "--update") == 0) cfg->operation_mode = OPERATION_MODE_UPDATE;

    else if(strcmp(arg, "--convert") == 0) cfg->convert = true;
    else if(strcmp(arg, "--verbose") == 0) cfg->verbose = true;
    else if(strcmp(arg, "--ignore-resources") == 0)
    {
        add_filter(&cfg->blacklist, "-b=*.stex");
        add_filter(&cfg->blacklist, "-b=*.image");
        add_filter(&cfg->blacklist, "-b=*.res");
        add_filter(&cfg->blacklist, "-b=*.texarr");
        add_filter(&cfg->blacklist, "-b=*.tex3d");
    }
    else if(strcmp(arg, "--help") == 0) print_help_message();

    else
    {
        printf("gdpc: Unknown option '%s'\nTry 'gdpc --help' for more information.\n", arg);
        return 1;
    }

    return 0;
}

static int parse_short_options(char* arg, config* cfg)
{
    // For each option...
    for(int i = 1; arg[i] != '\0'; ++i)
    {
        switch(arg[i])
        {
            case 'l': cfg->operation_mode = OPERATION_MODE_LIST;
                      break;
            case 'e': cfg->operation_mode = OPERATION_MODE_EXTRACT;
                      break;
            case 'c': cfg->operation_mode = OPERATION_MODE_CREATE;
                      break;
            case 'u': cfg->operation_mode = OPERATION_MODE_UPDATE;
                      break;

            case 'v': cfg->verbose = true;
                      break;

            case 'i': add_filter(&cfg->blacklist, "-b=*.stex");
                      add_filter(&cfg->blacklist, "-b=*.image");
                      add_filter(&cfg->blacklist, "-b=*.res");
                      add_filter(&cfg->blacklist, "-b=*.texarr");
                      add_filter(&cfg->blacklist, "-b=*.tex3d");
            
            case 'h': print_help_message();
                      break;
            
            default: printf("gdpc: Unknown option '%c' in '%s'\nTry 'gdpc --help' for more information.\n", arg[i], arg);
                     return 1;
        }
    }

    return 0;
}

static int parse_value(char* arg, config* cfg)
{
    switch(arg[1])
    {
        case 'v': sscanf(arg, "-v=%d.%d.%d", &cfg->version_major, &cfg->version_minor, &cfg->version_revision);
                  break;
        
        case 'w': return add_filter(&cfg->whitelist, arg);
                  break;
        case 'b': return add_filter(&cfg->blacklist, arg);

        default: printf("gdpc: Unknown option '-%c'\nTry 'gdpc --help' for more information.\n", arg[1]);
                 return 1;
    }

    return 0;
}

static int add_filter(dynamic_array* arr, char* arg)
{
    filter fil;

    // Allocate memory
    size_t len = strlen(arg) - 3; // Ignore "-w="
    fil.data = malloc(len + 1);
    fil.end = fil.data + len + 1;

    if(fil.data == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }
    strcpy(fil.data, arg + 3);

    // Find wildcard character
    fil.wildcard = NULL;
    for(char* itr = fil.data; *itr != '\0'; ++itr)
    {
        if(*itr == '*')
        {
            fil.wildcard = itr;
            break;
        }
    }

    // Push back
    dynamic_array_push_back(arr, &fil);

    return 0;
}

static int parse_paths(char* arg, config* cfg)
{
    // Copy path
    size_t len = strlen(arg);
    char* data = malloc(len + 1);
    strcpy(data, arg);

    dynamic_array_push_back(&cfg->input_files, &data);

    return 0;
}

static void print_help_message()
{
    printf("usage: gdpc [-aceiluv] [--longoption ...] [[file ...] dest]\n");
    exit(0);
}

void free_config(config* cfg)
{
    // Free the heap allocated strings from the file list
    char** files = (char**)cfg->input_files.data;
    for(size_t i = 0; i < cfg->input_files.size; ++i) 
    {
        free(files[i]);
    }

    // Free the heap allocated strings contained in the filters
    for(size_t i = 0; i < cfg->whitelist.size; ++i)
    {
        filter* f = (filter*)&cfg->whitelist.data[i * sizeof(filter)];
        free(f->data);
    }

    for(size_t i = 0; i < cfg->blacklist.size; ++i)
    {
        filter* f = (filter*)&cfg->blacklist.data[i * sizeof(filter)];
        free(f->data);
    }

    dynamic_array_free(&cfg->input_files);
    dynamic_array_free(&cfg->whitelist);
    dynamic_array_free(&cfg->blacklist);
    free(cfg->destination);
}