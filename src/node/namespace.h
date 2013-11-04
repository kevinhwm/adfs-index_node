/* namespace.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <pthread.h>
#include <kclangc.h>
#include "../adfs.h"

#define NODE_MAX_FILE_NUM       50000


typedef enum ADFS_NODE_STATE 
{
    S_LOST		= 0,
    S_READ_ONLY,
    S_READ_WRITE,
    //S_SYN
}ADFS_NODE_STATE;

typedef struct NodeDB
{
    int             id;
    ADFS_NODE_STATE state;
    char            path[ADFS_MAX_LEN];
    KCDB        *   db;
    unsigned long   count;

    struct NodeDB * prev;
    struct NodeDB * next;
}NodeDB;

typedef struct ANNameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    unsigned long number;
    pthread_rwlock_t lock;

    struct NodeDB * head;
    struct NodeDB * tail;
    struct ANNameSpace * prev;
    struct ANNameSpace * next;

    // functions
    void (*release)(struct ANNameSpace *);
    int (*create)(struct ANNameSpace *, const char *path, const char *args, ADFS_NODE_STATE);
    struct NodeDB * (*get)(struct ANNameSpace *, int);
    int (*needto_split)(struct ANNameSpace * _this);
    int (*split_db)(struct ANNameSpace * _this, const char *path, const char *args);
    void (*count_add)(struct ANNameSpace * _this);
}ANNameSpace;

// namespace.c
int anns_init(ANNameSpace *_this, const char * name_space);

#endif // __NAMESPACE_H__

