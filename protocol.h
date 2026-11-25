#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * @file protocol.h
 * @brief NetBoX protocol specification.
 * 
 * Includes only limits and sizes defined by the protocol specified in the 
 * project statement. Other limits and sizes that consist on implementation 
 * choices, such as buffer sizes and timeouts, are defined in their own modules 
 * instead.
 */

// User credentials.
// Sent in LIN, LOU, UNR, PUB and REM (UID also in WHR, and returned in RVR).
#define UID_LEN             6         // Exactly 6 digits.
#define PWD_LEN             8         // Exactly 8 alphanumeric characters.

// Files.
// Filenames: sent in PUB, REM, VRS, WHR and GET, and returned in RLS.
// Sizes (Fsize): sent in PUB, and returned in RVR and RGT.
#define FILENAME_MAX_LEN    24                  // Longest valid filename (name, dot and 3-letter extension).
#define FILENAME_EXT_LEN    3                   // Extension length (after the dot).
#define MAX_FILESIZE        (10 * 1000 * 1000)  // 10 MB (10 * 10^6 bytes).
#define FSIZE_MAX_DIGITS    8

// Ports.
// Startup arguments (-m, -p), the peer TCP port sent in LIN, and returned in RWH.
#define PORT_MIN            1
#define PORT_MAX            65535
#define PORT_STR_MAX_LEN    5

// Labels (publish).
// Sent in PUB, and returned in RVR.
#define LABEL_MIN_LEN       1
#define LABEL_MAX_LEN       20

// Other command-specific limits.
#define MAX_LIST_ENTRIES    50                  // list: maximum number of filenames in an RLS reply.
#define AVAILABILITY_LEN    3                   // versions: "AVL" or "NAV" in each RVR entry.

#endif // PROTOCOL_H