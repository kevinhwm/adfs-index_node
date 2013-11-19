/* namespace.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <pthread.h>
#include <kclangc.h>
#include "../def.h"

#define NODE_MAX_FILE_NUM       50000


typedef struct NodeDB
{
    int             	id;
    _DFS_NODE_STATE 	state;
    char            	path[_DFS_MAX_LEN];
    KCDB              * db;
    unsigned long   	count;

    struct NodeDB     *	prev;
    struct NodeDB     *	next;
}NodeDB;

typedef struct CNNameSpace
{
    pthread_rwlock_t 	lock;
    char 		name[_DFS_NAMESPACE_LEN];
    unsigned long 	number;
    KCDB 	      *	index_db;

    struct NodeDB * head;
    struct NodeDB * tail;
    struct CNNameSpace * prev;
    struct CNNameSpace * next;

    // functions
    void 		(*release)(struct CNNameSpace *);
    int 		(*create)(struct CNNameSpace *, const char *path, const char *args, _DFS_NODE_STATE);
    struct NodeDB * 	(*get)(struct CNNameSpace *, int);
    int 		(*needto_split)(struct CNNameSpace * _this);
    int 		(*split_db)(struct CNNameSpace * _this, const char *path, const char *args);
    void 		(*count_add)(struct CNNameSpace * _this);
}CNNameSpace;

// namespace.c
int GNns_init(CNNameSpace *_this, const char *name_space, const char *data_dir, const char *db_args);

#endif // __NAMESPACE_H__

