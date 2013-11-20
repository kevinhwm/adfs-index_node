/* zone.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __ZONE_H__
#define __ZONE_H__

#include <pthread.h>
#include <curl/curl.h>
#include "../def.h"


#define _DFS_NODE_CURL_NUM  5

typedef enum {
    FLAG_INIT		= 0,
    FLAG_UPLOAD		= 1,
    FLAG_ERASE,
    FLAG_STATUS,
} FLAG_CONNECTION;

typedef struct {
    struct {
	CURL *curl;
	pthread_mutex_t *mutex;
	FLAG_CONNECTION flag;
    } conn[ _DFS_NODE_CURL_NUM ];

    char name[ _DFS_NODENAME_LEN ];
    char ip_port[ _DFS_NODENAME_LEN ];
    _DFS_NODE_STATE state;
    pthread_mutex_t *lock;

    struct CINode *prev;
    struct CINode *next;
}CINode;

typedef struct {
    char name[_DFS_ZONENAME_LEN];
    int num;
    double weight;
    double count;

    struct CINode *head;
    struct CINode *tail;
    struct CIZone *prev;
    struct CIZone *next;
    // function
    int (*create)(struct CIZone *, const char *, const char *, const char *);
    void (*release)(struct CIZone *);
    CINode * (*rand_choose)(struct CIZone *);
}CIZone;

int GIz_init(CIZone *_this, const char *name, int weight);

#endif // __ZONE_H__

