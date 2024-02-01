#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_io.h"
#include "logger.h"

void
editor_fopen(econf_t *config, char *filename)
{
    // Free filename from config when opening new file
    free(config->filename);
    config->filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");
}
