#include "gdpc.h"
#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv)
{
    // Parse command line arguments
    config cfg;

    if(parse_command_line_arguments(argc, argv, &cfg) != 0)
    {
        return 1;
    }

    // If listing or extracting
    if(cfg.operation_mode == OPERATION_MODE_LIST || cfg.operation_mode == OPERATION_MODE_EXTRACT)
    {
        // Read the packs
        if(read_packs(&cfg) != 0)
        {
            return 1;
        }
    }
    // Else if creating or updating
    else if(cfg.operation_mode == OPERATION_MODE_CREATE || cfg.operation_mode == OPERATION_MODE_UPDATE)
    {

    }

    return 0;
}