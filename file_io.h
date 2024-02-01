/**
 * @file file_io.h
 * @author re-nanashi
 * @brief File I/O operations
 */

#ifndef FILE_IO_H
#define FILE_IO_H

#include "editor_config.h"

/**
 * @brief Extract the window size of the terminal
 *
 * @param filename Pointer to the (FILE *) variable that was passed
 */
void editor_fopen(econf_t *config, char *filename);

#endif /* FILE_IO_H */
