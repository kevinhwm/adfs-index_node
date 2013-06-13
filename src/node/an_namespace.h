/* an_namespace.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <pthread.h>
#include <kclangc.h>
#include "../include/adfs.h"

#define NODE_MAX_FILE_NUM       100000


typedef enum ADFS_NODE_STATE 
{
    S_LOST		= 0,
    S_READ_ONLY,
    S_READ_WRITE,
    S_SYN
}ADFS_NODE_STATE;

typedef struct NodeDB
{
    int             id;
    ADFS_NODE_STATE state;
    char            path[ADFS_MAX_PATH];
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
    int sign_syn;

    struct NodeDB * head;
    struct NodeDB * tail;
    struct ANNameSpace * prev;
    struct ANNameSpace * next;

    // functions
    void (*release)(struct ANNameSpace *);
    ADFS_RESULT (*create)(struct ANNameSpace *, const char *path, const char *args, ADFS_NODE_STATE);
    struct NodeDB * (*get)(struct ANNameSpace *, int);
    int (*needto_split)(struct ANNameSpace * _this);
    ADFS_RESULT (*split_db)(struct ANNameSpace * _this, const char *path, const char *args);
<<<<<<< HEAD
    void (*count_add)(struct ANNameSpace *_this);
    ADFS_RESULT (*switch_state)(struct ANNameSpace *_this, ADFS_NODE_STATE state);
=======
    void (*count_add)(struct ANNameSpace * _this);
    void (*syn)(struct ANNameSpace * _this);
>>>>>>> master
}ANNameSpace;

// an_namespace.c
ADFS_RESULT anns_init(ANNameSpace *_this, const char * name_space);

#endif // __NAMESPACE_H__

