#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void print_help_message();
static int parse_long_option(char* arg, config* cfg);
static int parse_short_options(char* arg, config* cfg);
static int parse_paths(char* arg, config* cfg);

bool parse_command_line_arguments(int argc, char** argv, config* cfg)
{
    // Default initialize the configuration
    cfg->verbose = false;
    cfg->operation_mode = OPERATION_MODE_UNSPECIFIED;
    cfg->input_count = 0;
    cfg->input_files = NULL;
    cfg->destination = NULL;

    // For each argument, except the last one
    for(int i = 1; i < argc; ++i)
    {
        // If the argument is a long option...
        if(argv[i][0] == '-' && argv[i][1] == '-')
        {
            if(parse_long_option(argv[i], cfg) != 0) return 1;
        }
        // Else, if it's a short option...
        else if(argv[i][0] == '-' && argv[i][1] != '-')
        {
            if(parse_short_options(argv[i], cfg) != 0) return 1;
        }
        // If the argument is not an option, parse all paths
        else
        {
            // Parse all input files
            for(int j = 0; j < argc - (i + 1); ++j)
            {
                parse_paths(argv[i + j], cfg);
            }

            // If operation mode is LIST, parse last path as input
            if(cfg->operation_mode == OPERATION_MODE_LIST)
            {
                parse_paths(argv[argc - 1], cfg);
            }
            else
            {
                size_t len = strlen(argv[argc - 1]);
                cfg->destination = malloc(len + 1);

                if(cfg->destination == NULL)
                {
                    fprintf(stderr, "malloc(): failed to allocate memory.\n");
                    abort();
                }

                strcpy(cfg->destination, argv[argc - 1]);
            }

            break;
        }
    }

    // Error checking
    if(cfg->operation_mode == OPERATION_MODE_UNSPECIFIED)
    {
        printf("gdpc: You must specify the operation mode.\nTry 'gdpc --help' for more information.\n");
        return 1;
    }
    if(cfg->input_files == NULL && cfg->operation_mode != OPERATION_MODE_LIST)
    {
        printf("gdpc: You must provide files to extract/archive.\nTry 'gdpc --help' for more information.\n");
        return 1;
    }

    return 0;
}

static int parse_long_option(char* arg, config* cfg)
{
    if(strcmp(arg, "--list") == 0) cfg->operation_mode = OPERATION_MODE_LIST;
    else if(strcmp(arg, "--extract") == 0) cfg->operation_mode = OPERATION_MODE_EXTRACT;
    else if(strcmp(arg, "--create") == 0) cfg->operation_mode = OPERATION_MODE_CREATE;
    else if(strcmp(arg, "--update") == 0) cfg->operation_mode = OPERATION_MODE_UPDATE;

    else if(strcmp(arg, "--verbose") == 0) cfg->verbose = true;
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
            
            default: printf("Unknown option '%c' in '%s'\nTry 'gdpc --help' for more information.\n", arg[i], arg);
                     return 1;
        }
    }

    return 0;
}


// This seems needlessly complicated but will be needed when support for wildcard expansion will be added
static int parse_paths(char* arg, config* cfg)
{
    // Increase the size of the file array by 1
    if(cfg->input_count == 0) cfg->input_files = malloc(sizeof(char*));
    else                      cfg->input_files = realloc(cfg->input_files, cfg->input_count + 1);

    if(cfg->input_files == NULL)
    {
        if(cfg->input_count == 0) fprintf(stderr, "malloc(): failed to allocate memory.\n");
        else                      fprintf(stderr, "realloc(): failed to re-allocate memory.\n");
        abort();
    }

    cfg->input_count++;

    // Add the path
    size_t len = strlen(arg);
    cfg->input_files[cfg->input_count - 1] = malloc(len + 1);

    if(cfg->input_files[cfg->input_count - 1] == NULL)
    {
        fprintf(stderr, "malloc(): failed to allocate memory.\n");
        abort();
    }

    strcpy(cfg->input_files[cfg->input_count - 1], arg);

    return 0;
}

static void print_help_message()
{
    printf("usage: gdpc [OPTIONS...] [FILES...] [DEST]\n");
    exit(0);
}