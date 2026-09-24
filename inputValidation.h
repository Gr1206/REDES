#ifndef INPUTVALIDATION_H
#define INPUTVALIDATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_FILESIZE (10LL * 1000 * 1000) //MANTEM EM 64 BITS

/**
 * @brief Check if the UID has 6 digits
 * @param uid The UID to check
 * @return 1 if the UID is valid, 0 invalid 
 */
int checkUID(char* uid);

/**
 * @brief Check if the password has 8 alphanumeric chars
 * @param password The password to check
 * @return 1 if the password is valid, 0 invalid
 */
int checkPassword(char* password);

/**
 * @brief Check if the port is in the range 1-65535
 * @param port The port to check
 * @return 1 if the port is valid, 0 invalid
 */
int checkPort(int port);

/**
 * @brief Check if the character is a valid (Letter, Number, - or _)
 * @param c The character to check
 * @return 1 if the character is valid, 0 invalid
 */
int checkChar(char c);

/**
 * @brief Check if the filename is valid
 * @param filename The filename to check
 * @return 1 if the filename is valid, 0 invalid
 */
int checkFilename(char* filename);

/**
 * @brief Check if the label is valid
 * @param label The label to check
 * @return 1 if the label is valid, 0 invalid
 */
int checkLabel(char* label);

/**
 * @brief Check if the file size is valid (0 to MAX_FILESIZE)
 * @param size The file size to check
 * @return 1 if the file size is valid, 0 invalid
 */
int checkFileSize(off_t size);

/**
 * @brief Check if a file exists
 * @param path The path to the file
 * @return -1 if the file does not exist, -2 if the file size is invalid, 
 *          or filesize in case it is valid
 */
int fileExists(const char *path);
    

#endif 
