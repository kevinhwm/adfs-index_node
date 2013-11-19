/* record.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __RECORD_H__
#define __RECORD_H__

#include "../def.h"


// format: "uuidzone#node|zone#node$uuidzone#node|zone#node..."
// zone#node						-> a position
// zone#node|zone#node					-> multiple positions
// uuidzone#node|zone#node				-> a record. length of uuid is exactly 24 bytes
// uuidzone#node|zone#node$uuidzone#node|zone#node	-> that real records look like

typedef struct CIPosition
{
    char zone_node[_DFS_ZONENAME_LEN + _DFS_NODENAME_LEN];

    struct CIPosition *prev;
    struct CIPosition *next;
}CIPosition;

// record: uuid$zone#node|zone#node
typedef struct CIRecord
{
    char uuid[_DFS_UUID_LEN + 8];
    int num;
    struct CIPosition *head;
    struct CIPosition *tail;
    // function
    int (*add)(struct CIRecord *, const char *, const char *);
    void (*release)(struct CIRecord *);
    char * (*get_string)(struct CIRecord *);
}CIRecord;

void air_init(CIRecord *);

#endif // __RECORD_H__

