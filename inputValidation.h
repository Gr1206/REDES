#ifndef INPUTVALIDATION_H
#define INPUTVALIDATION_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#include "protocol.h"

// getLocalFileSize()-specific constants.
#define LOCAL_FILE_OK           0
#define LOCAL_FILE_NOT_FOUND   -1
#define LOCAL_FILE_NOT_REGULAR -2   // e.g., a directory named "dir.txt"
#define LOCAL_FILE_TOO_LARGE   -3

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
 * @brief Check if the filename is valid : 24 chars & only letters, digits, - or _
 * and check if extension has 3 chars
 * @param filename The filename to check
 * @return 1 if the filename is valid, 0 invalid
 */
int checkFilename(char* filename);

/**
 * @brief Check if the label is valid : 20 chars max & only letters, digits, - or _
 * @param label The label to check
 * @return 1 if the label is valid, 0 invalid
 */
int checkLabel(char* label);

/**
 * @brief Check if the file size is valid (0 to MAX_FILESIZE) : 10 000 000 bytes
 * @param size The file size to check
 * @return 1 if the file size is valid, 0 invalid
 */
int checkFileSize(off_t size);

/**
 * @brief Gets the size of a local file
 *
 * Only files within the protocol's size limit are accepted.
 * 
 * @param path The path to the file
 * @param size The file size in bytes, set only on LOCAL_FILE_OK
 * @return LOCAL_FILE_OK, LOCAL_FILE_NOT_FOUND, LOCAL_FILE_NOT_REGULAR or 
 *         LOCAL_FILE_TOO_LARGE
 */
int getLocalFileSize(const char *path, long *size);

/**
 * @brief Check if a string is exactly a valid port (1-65535)
 * @param s The string to check
 * @param port The parsed port, to be set only if valid
 * @return 1 if valid, 0 invalid
 */
int checkPortStr(const char *s, int *port);


#endif 
