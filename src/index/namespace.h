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


typedef struct CISynPrim {
    pthread_t th_cls; 

    struct {
	pthread_mutex_t lock; 
	int num;
	FILE *f_inc; 
	char f_name[512]; 
    } syn_prim;
}CISynPrim;


typedef struct CISynSec {
}CISynSec;


typedef struct CINameSpace {
    char name[ _DFS_NAMESPACE_LEN ];
    KCDB *index_db;

    struct CINameSpace *prev;
    struct CINameSpace *next;
}CINameSpace;


// syn_primary.c
int GIsp_init();
int GIsp_release();
int GIsp_export(const char *, const char *, const char *, const char *);

// syn_secondary.c
int GIss_init();
int GIss_release();

#endif // __NAMESPACE_H__

