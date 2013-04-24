/* Antiy Labs. Basic Platform R & D Center
 * ai_zone.c
 *
 * huangtao@antiy.com
 */

#include <stdlib.h>
#include <string.h>
#include "ai.h"

static ADFS_RESULT z_create(AIZone *_this, const char *ip_port);
static void z_release_all(AIZone *_this);
static AINode * z_rand_choose(AIZone *_this);

ADFS_RESULT z_init(AIZone *_this, const char *name, int weight)
{
    if (_this)
    {
        memset(_this, 0, sizeof(AIZone));
        strncpy(_this->name, name, sizeof(_this->name));
        _this->weight = weight;
        _this->num = 0;

        _this->create = z_create;
        _this->release_all = z_release_all;
        _this->rand_choose = z_rand_choose;
        return ADFS_OK;
    }
    return ADFS_ERROR;
}

static ADFS_RESULT z_create(AIZone *_this, const char *ip_port)
{
    AINode *pn = _this->head;
    while (pn)
    {
        if (strcmp(pn->ip_port, ip_port) == 0)
            return ADFS_ERROR;
        pn = pn->next;
    }

    AINode *new_node = (AINode *)malloc(sizeof(AINode));
    if (new_node == NULL)
        return ADFS_ERROR;

    strncpy(new_node->ip_port, ip_port, sizeof(new_node->ip_port));
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i)
    {
        new_node->curl[i] = curl_easy_init();
        if (new_node->curl[i] == NULL)
            return ADFS_ERROR;

        if ( pthread_mutex_init(new_node->curl_mutex+i, NULL) != 0)
            return ADFS_ERROR;
    }

    _this->num += 1;

    new_node->pre = _this->tail;
    new_node->next = NULL;

    if (_this->tail)
        _this->tail->next = new_node;
    else
        _this->head = new_node;
    _this->tail = new_node;

    return ADFS_OK;
}

static void z_release_all(AIZone *_this)
{
    while (_this->tail)
    {
        AINode *pn = _this->tail;
        _this->tail = _this->tail->pre;

        for (int i=0; i<ADFS_NODE_CURL_NUM; ++i)
        {
            curl_easy_cleanup(pn->curl[i]);
            pn->curl[i]= NULL;

            pthread_mutex_destroy(pn->curl_mutex + i);
        }
        free(pn);
    }

    
}

static AINode * z_rand_choose(AIZone *_this)
{
    int n = rand()%(_this->num);
    AINode *pn = _this->head;
    for (int i=0; i<n; ++i)
        pn = pn->next;

    return pn;
}

