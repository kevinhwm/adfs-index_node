/* 
 * record.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __RECORD_H__
#define __RECORD_H__

#include "def.h"

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
    void (*create_uuid)(struct AIRecord *);
    ADFS_RESULT (*add)(struct AIRecord *, const char *, const char *);
    void (*release)(struct AIRecord *);
    char * (*get_string)(struct AIRecord *);
    //ADFS_RESULT (*parse_string)(const char *);
}AIRecord;

void air_init(AIRecord *);

#endif // __RECORD_H__

