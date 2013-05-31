/* an_namespace.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include "../include/adfs.h"
#include <kclangc.h>

#define NODE_MAX_FILE_NUM       100000


typedef enum ADFS_NODE_STATE
{
    S_READ_ONLY     = 0,
    S_READ_WRITE    = 1,
}ADFS_NODE_STATE;

typedef struct NodeDB
{
    int             id;
    ADFS_NODE_STATE state;
    char            path[ADFS_MAX_PATH];
    unsigned long   count;
    KCDB        *   db;

    struct NodeDB * pre;
    struct NodeDB * next;
}NodeDB;

typedef struct ANNameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    unsigned long number;

    struct NodeDB * head;
    struct NodeDB * tail;
    struct ANNameSpace * pre;
    struct ANNameSpace * next;

    // functions
    ADFS_RESULT (*create)(struct ANNameSpace *, const char *path, const char *args, ADFS_NODE_STATE);
    void (*release_all)(struct ANNameSpace *);
    struct NodeDB * (*get)(struct ANNameSpace *, int);
    ADFS_RESULT (*switch_state)(struct ANNameSpace *, int, ADFS_NODE_STATE);
}ANNameSpace;

// an_namespace.c
ADFS_RESULT anns_init(ANNameSpace *_this, const char * name_space);

#endif // __NAMESPACE_H__

