/* ai_zone.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>	// malloc
#include <string.h>
#include "ai_zone.h"

static ADFS_RESULT z_create(AIZone *_this, const char *name, const char *ip_port);
static void z_release(AIZone *_this);
static AINode * z_rand_choose(AIZone *_this);

ADFS_RESULT aiz_init(AIZone *_this, const char *name, int weight)
{
    if (_this) {
        memset(_this, 0, sizeof(AIZone));
        strncpy(_this->name, name, sizeof(_this->name));
        _this->weight = weight;
        _this->num = 0;

        _this->create = z_create;
        _this->release = z_release;
        _this->rand_choose = z_rand_choose;
        return ADFS_OK;
    }
    return ADFS_ERROR;
}

static ADFS_RESULT z_create(AIZone *_this, const char *name, const char *ip_port)
{
    for (AINode *pn = _this->head; pn; pn = pn->next) {
        if (strcmp(pn->ip_port, ip_port) == 0 || strcmp(pn->name, name) == 0) {return ADFS_ERROR;}
    }

    AINode *new_node = (AINode *)malloc(sizeof(AINode));
    if (new_node == NULL) {return ADFS_ERROR;}
    memset(new_node, 0, sizeof(AINode));
    strncpy(new_node->name, name, sizeof(new_node->name));
    strncpy(new_node->ip_port, ip_port, sizeof(new_node->ip_port));
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        new_node->conn[i].curl = curl_easy_init();
        if (new_node->conn[i].curl == NULL) {return ADFS_ERROR;}
	new_node->conn[i].mutex = malloc(sizeof(pthread_mutex_t));
        if (pthread_mutex_init(new_node->conn[i].mutex, NULL) != 0) {return ADFS_ERROR;}
	new_node->conn[i].flag = FLAG_INIT;
    }
    _this->num += 1;

    new_node->prev = _this->tail;
    new_node->next = NULL;
    if (_this->tail) {_this->tail->next = new_node;}
    else {_this->head = new_node;}
    _this->tail = new_node;
    return ADFS_OK;
}

static void z_release(AIZone *_this)
{
    while (_this->tail) {
        AINode *pn = _this->tail;
        _this->tail = _this->tail->prev;

        for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
            curl_easy_cleanup(pn->conn[i].curl);
            pn->conn[i].curl = NULL;
            pthread_mutex_destroy(pn->conn[i].mutex);
	    free(pn->conn[i].mutex);
        }
        free(pn);
    }
}

static AINode * z_rand_choose(AIZone *_this)
{
    int n = rand()%(_this->num);
    AINode *pn = _this->head;
    for (int i=0; i<n; ++i) {pn = pn->next;}
    return pn;
}

