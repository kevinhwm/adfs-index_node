/* ai_zone.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __ZONE_H__
#define __ZONE_H__

#include <pthread.h>
#include <curl/curl.h>
#include "../adfs.h"

#define ADFS_NODE_CURL_NUM  5

typedef enum 
{
    FLAG_INIT		= 0,
    FLAG_UPLOAD		= 1,
    FLAG_ERASE,
    FLAG_STATUS,
    FLAG_SYN,
} FLAG_CONNECTION;

typedef struct AINode
{
    struct {
	CURL *curl;
	pthread_mutex_t *mutex;
	FLAG_CONNECTION flag;
    } conn[ADFS_NODE_CURL_NUM];
    char name[ADFS_NODENAME_LEN];
    char ip_port[ADFS_NODENAME_LEN];

    struct AINode *prev;
    struct AINode *next;
}AINode;

typedef struct AIZone
{
    char name[ADFS_ZONENAME_LEN];
    int num;
    double weight;
    double count;

    struct AINode *head;
    struct AINode *tail;
    struct AIZone *prev;
    struct AIZone *next;
    // function
    ADFS_RESULT (*create)(struct AIZone *, const char *, const char *);
    void (*release)(struct AIZone *);
    AINode * (*rand_choose)(struct AIZone *);
}AIZone;

ADFS_RESULT aiz_init(AIZone *_this, const char *name, int weight);

#endif // __ZONE_H__
