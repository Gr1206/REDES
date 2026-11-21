#ifndef USER_H
#define USER_H

/**
 * @file user.h
 * @brief Representation of the currently logged-in user.
 */

#include "protocol.h"

/**
 * @brief State of the user session.
 *
 * The client's local record of a logged in user session, populated upon a 
 * successful login and cleared on logout.
 */
typedef struct User {
    char UID[UID_LEN + 1];      /**< 6-digit UID, plus null terminator. */
    char password[PWD_LEN + 1]; /**< 8-char password, plus null terminator. */
    int loggedIn;               /**< 1 if currently logged in, 0 otherwise. */
} User;

#endif // USER_H