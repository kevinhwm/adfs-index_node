/* zone.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>	// malloc
#include <string.h>
#include "zone.h"

static int z_create(AIZone *_this, const char *name, const char *state, const char *ip_port);
static void z_release(AIZone *_this);
static AINode * z_rand_choose(AIZone *_this);

int aiz_init(AIZone *_this, const char *name, int weight)
{
    if (_this) {
        memset(_this, 0, sizeof(AIZone));
        strncpy(_this->name, name, sizeof(_this->name));
        _this->weight = weight;
        _this->num = 0;

        _this->create = z_create;
        _this->release = z_release;
        _this->rand_choose = z_rand_choose;
        return 0;
    }
    return -1;
}

static int z_create(AIZone *_this, const char *name, const char *state, const char *ip_port)
{
    for (AINode *pn = _this->head; pn; pn = pn->next) {
        if (strcmp(pn->ip_port, ip_port) == 0 || strcmp(pn->name, name) == 0) { return -1; }
    }

    AINode *new_node = (AINode *)malloc(sizeof(AINode));
    if (new_node == NULL) { return -1; }
    memset(new_node, 0, sizeof(AINode));
    strncpy(new_node->name, name, sizeof(new_node->name));
    strncpy(new_node->ip_port, ip_port, sizeof(new_node->ip_port));
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        new_node->conn[i].curl = curl_easy_init();
        if (new_node->conn[i].curl == NULL) { return -1; }
	new_node->conn[i].mutex = malloc(sizeof(pthread_mutex_t));
        if (pthread_mutex_init(new_node->conn[i].mutex, NULL) != 0) { return -1; }
	new_node->conn[i].flag = FLAG_INIT;
    }
    if (strcmp(state, "rw") == 0) {new_node->state = S_READ_WRITE;}
    else if (strcmp(state, "ro") == 0) {new_node->state = S_READ_ONLY;}
    else {new_node->state = S_NA;}
    new_node->lock = malloc(sizeof(pthread_mutex_t));
    if (pthread_mutex_init(new_node->lock, NULL) != 0) {return -1;}

    new_node->prev = _this->tail;
    new_node->next = NULL;
    if (_this->tail) {_this->tail->next = new_node;}
    else {_this->head = new_node;}
    _this->tail = new_node;

    _this->num += 1;
    return 0;
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

