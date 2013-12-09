/* namespace.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __NAMESPACE_H__
#define __NAMESPACE_H__

#include <pthread.h>
#include <stdlib.h>
#include <kclangc.h>
#include "../def.h"


typedef struct CINsSec {
    pthread_t th_get;
    int num;
    int line;
}CINsSec;

typedef struct CINsPrim {
    pthread_mutex_t lock; 
    int num;
    FILE *f_inc; 
    char f_name[512]; 

    int (*output)(struct CINsPrim *_this);
}CINsPrim;


typedef struct CINameSpace {
    char name[ _DFS_NAMESPACE_LEN ];
    KCDB *index_db;

    struct CINsPrim prim;
    struct CINsSec sec;

    struct CINameSpace *prev;
    struct CINameSpace *next;

    int (*release)(struct CINameSpace *_this);
    int (*output)(struct CINameSpace *_this);
}CINameSpace;

int GIns_init(CINameSpace *_this, const char *name, const char *db_args, int primary);

#endif // __NAMESPACE_H__

