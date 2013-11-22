/* zone.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>	// malloc
#include <string.h>
#include "zone.h"

static void z_release(CIZone *_this);
static int z_create(CIZone *_this, const char *name, const char *ip_port, const char *state);
static CINode ** z_get_nodelist(CIZone *_this, int *pnum);

int GIz_init(CIZone *_this, const char *name)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CIZone));
    strncpy(_this->name, name, sizeof(_this->name));
    _this->num = 0;

    _this->create = z_create;
    _this->release = z_release;
    _this->get_nodelist = z_get_nodelist;
    return 0;
}

static void z_release(CIZone *_this)
{
    while (_this->tail) {
        CINode *pn = _this->tail;
        _this->tail = _this->tail->prev;

        for (int i=0; i<_DFS_NODE_CURL_NUM; ++i) {
            curl_easy_cleanup(pn->conn[i].curl);
            pn->conn[i].curl = NULL;
            pthread_mutex_destroy(pn->conn[i].mutex);
	    free(pn->conn[i].mutex);
        }
	pthread_mutex_destroy(pn->lock);
	free(pn->lock);
        free(pn);
    }
}

static int z_create(CIZone *_this, const char *name, const char *ip_port, const char *state)
{
    for (CINode *pn = _this->head; pn; pn = pn->next) {
        if (strcmp(pn->ip_port, ip_port) == 0 || strcmp(pn->name, name) == 0) { return -1; }
    }

    CINode *new_node = (CINode *)malloc(sizeof(CINode));
    if (new_node == NULL) { return -1; }
    memset(new_node, 0, sizeof(CINode));
    strncpy(new_node->name, name, sizeof(new_node->name));
    strncpy(new_node->ip_port, ip_port, sizeof(new_node->ip_port));
    for (int i=0; i<_DFS_NODE_CURL_NUM; ++i) {
        if ((new_node->conn[i].curl = curl_easy_init()) == NULL) { return -1; }
	new_node->conn[i].mutex = malloc(sizeof(pthread_mutex_t));
        if (pthread_mutex_init(new_node->conn[i].mutex, NULL) != 0) { return -1; }
	new_node->conn[i].flag = FLAG_INIT;
    }
    if (strcmp(state, "rw") == 0) { new_node->state = S_READ_WRITE; }
    else if (strcmp(state, "ro") == 0) { new_node->state = S_READ_ONLY; }
    else {new_node->state = S_NA;}
    new_node->lock = malloc(sizeof(pthread_mutex_t));
    if (pthread_mutex_init(new_node->lock, NULL) != 0) { return -1; }

    new_node->prev = _this->tail;
    new_node->next = NULL;
    if (_this->tail) { _this->tail->next = new_node; }
    else {_this->head = new_node;}
    _this->tail = new_node;

    _this->num += 1;
    return 0;
}

static CINode ** z_get_nodelist(CIZone *_this, int *pnum)
{
    CINode **node_list = malloc(_this->num * sizeof(CINode *));
    if (node_list == NULL) { return NULL; }
    *pnum = 0;

    CINode *pn = _this->head;
    while (pn) {
	if (pn->state == S_READ_WRITE) {
	    memcpy(node_list+*pnum, &pn, sizeof(CINode *));
	    (*pnum)++;
	    if (*pnum > _this->num) { free(node_list); return NULL; }
	}
	pn = pn->next;
    }
    return node_list;
}

