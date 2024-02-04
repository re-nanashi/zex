/**
 * @file file_io.h
 * @author re-nanashi
 * @brief File I/O operations
 */

#ifndef FILE_IO_H
#define FILE_IO_H

/**
 * @brief Converts text from rows to one big string
 *
 * @param buflen Pointer to length variable
 */
char *editor_rows_to_string(int *buflen);

/**
 * @brief Opens file and extracts text to be drawn to editor
 *
 * @param filename Pointer to the (FILE *) variable that was passed
 */
void editor_fopen(char *filename);

/* @brief Saves the currently rendered text in the editor to a file */
void editor_save();

#endif /* FILE_IO_H */
