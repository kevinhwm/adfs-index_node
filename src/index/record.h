/* 
 * record.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __RECORD_H__
#define __RECORD_H__

#include "def.h"


// format: "uuidzone#node|zone#node$uuidzone#node|zone#node..."
// zone#node						-> a position
// zone#node|zone#node					-> multiple positions
// uuidzone#node|zone#node				-> a record. length of uuid is exactly 24 bytes
// uuidzone#node|zone#node$uuidzone#node|zone#node	-> that real records look like

typedef struct AIPosition
{
    char zone_node[ADFS_ZONENAME_LEN + ADFS_NODENAME_LEN];

    struct AIPosition *pre;
    struct AIPosition *next;
}AIPosition;

// record: uuid$zone#node|zone#node
typedef struct AIRecord
{
    char uuid[ADFS_UUID_LEN + 8];
    int num;
    struct AIPosition *head;
    struct AIPosition *tail;
    // function
    ADFS_RESULT (*add)(struct AIRecord *, const char *, const char *);
    void (*release)(struct AIRecord *);
    char * (*get_string)(struct AIRecord *);
    //ADFS_RESULT (*parse_string)(const char *);
}AIRecord;

void air_init(AIRecord *);

#endif // __RECORD_H__

