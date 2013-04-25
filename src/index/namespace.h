/* 
 * namespace.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <linux/limits.h>

typedef struct AIPosition
{
    char zone[NAME_MAX];
    char node[NAME_MAX];

    struct AIPosition *pre;
    struct AIPosition *next;
}AIPosition;

typedef struct AIRecord
{
    char uuid[32];

    struct AIPosition *head;
    struct AIPosition *tail;

    struct AIRecord *pre;
    struct AIRecord *next;
}AIRecord;

typedef struct AINameSpace
{
    char name[NAME_MAX];

    // format: "uuidzone#node|zone#node$uuidzone#node|zone#node..."
    // zone#node						-> a position
    // zone#node|zone#node					-> multiple positions
    // uuidzone#node|zone#node					-> a record. length of uuid is exactly 24 bytes
    // uuidzone#node|zone#node$uuidzone#node|zone#node		-> that real records look like
    KCDB * index_db;

    struct AINameSpace *pre;
    struct AINameSpace *next;

    //ADFS_RESULT (*add)(uuid, zone, node);
    //const char * (*get)(int offset);
}AINameSpace;

AIRecord * parse(const char *record);

#endif // __NAMESPACE_H__

