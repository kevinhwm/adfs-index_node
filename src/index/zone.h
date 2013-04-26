/*
 * zone.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __ZONE_H__
#define __ZONE_H__

#include <pthread.h>
#include <curl/curl.h>

#define ADFS_NODE_CURL_NUM  4

typedef struct AINode
{
    char ip_port[ADFS_NODENAME_LEN];

    CURL *curl[ADFS_NODE_CURL_NUM];
    pthread_mutex_t curl_mutex[ADFS_NODE_CURL_NUM];

    struct AINode *pre;
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

    struct AIZone *pre;
    struct AIZone *next;

    ADFS_RESULT (*create)(struct AIZone *, const char *);
    void (*release_all)(struct AIZone *);
    AINode * (*rand_choose)(struct AIZone *);
}AIZone;


ADFS_RESULT z_init(AIZone *_this, const char *name, int weight);

#endif // __ZONE_H__
