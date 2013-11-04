/* namespace.c
 *
 * kevinhwm@gmail.com
 */

#include <pthread.h>
#include "namespace.h"

// list member functions. names begin with "ns_"
static void ns_release(ANNameSpace * _this);
static NodeDB * ns_get(ANNameSpace * _this, int id);
static int ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state);
static int ns_needto_split(ANNameSpace * _this);
static int ns_split_db(ANNameSpace * _this, const char *path, const char *args);
static void ns_count_add(ANNameSpace * _this);

// just functions
static int node_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state);
static int db_open(KCDB * db, char * path, ADFS_NODE_STATE state);


int anns_init(ANNameSpace * _this, const char *name_space)
{
    int res = -1;
    if (_this) {
        memset(_this, 0, sizeof(ANNameSpace));
        _this->release = ns_release;
        _this->get = ns_get;
        _this->create = ns_create;
	_this->needto_split = ns_needto_split;
	_this->split_db = ns_split_db;
	_this->count_add = ns_count_add;
	_this->number = 0;
	pthread_rwlock_init(&_this->lock, NULL);
        strncpy(_this->name, name_space, sizeof(_this->name));
        res = 0;
    }
    return res;
}

/////////////////////////////////////////////////////////////////////////////////
// use read-write lock
static void ns_release(ANNameSpace * _this)
{
    pthread_rwlock_destroy(&_this->lock);
    while (_this->tail) {
        NodeDB *tmp = _this->tail;
        _this->tail = _this->tail->prev;
        kcdbclose(tmp->db);
        kcdbdel(tmp->db);
        free(tmp);
    }
    return ;
}

static NodeDB * ns_get(ANNameSpace * _this, int id)
{
    pthread_rwlock_rdlock(&_this->lock);
    NodeDB * pn = _this->head;
    for (; pn; pn = pn->next) {
        if (pn->id == id) { pthread_rwlock_unlock(&_this->lock); return pn; }
    }
    pthread_rwlock_unlock(&_this->lock);
    return NULL;
}

static int ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state)
{
    pthread_rwlock_wrlock(&_this->lock);
    int res = node_create(_this, path, args, state);
    pthread_rwlock_unlock(&_this->lock);
    return res;
}

static int ns_needto_split(ANNameSpace * _this)
{
    pthread_rwlock_rdlock(&_this->lock);
    int res = _this->tail->count >= NODE_MAX_FILE_NUM;
    pthread_rwlock_unlock(&_this->lock);
    return res;
}

static int ns_split_db(ANNameSpace * _this, const char *path, const char *args)
{
    pthread_rwlock_wrlock(&_this->lock);
    if (_this->tail->count < NODE_MAX_FILE_NUM) { pthread_rwlock_unlock(&_this->lock); return 0; }

    NodeDB *pn = _this->tail;
    kcdbclose(pn->db);
    pn->state = S_READ_ONLY;
    if (db_open(pn->db, pn->path, pn->state) < 0) { pn->state = S_LOST; pthread_rwlock_unlock(&_this->lock); return -1; }
    node_create(_this, path, args, S_READ_WRITE);
    pthread_rwlock_unlock(&_this->lock);
    return 0;
}

static void ns_count_add(ANNameSpace * _this)
{
    pthread_rwlock_wrlock(&_this->lock);
    _this->tail->count++;
    pthread_rwlock_unlock(&_this->lock);
}

/////////////////////////////////////////////////////////////////////////////////
// not use read-write lock

static int node_create(ANNameSpace * pns, const char *path, const char *args, ADFS_NODE_STATE state)
{
    NodeDB * new_node = malloc(sizeof(NodeDB));
    if (new_node == NULL) {return -1;}
    
    new_node->id = pns->number;
    new_node->state = state;
    new_node->db = kcdbnew();

    char dbpath[ADFS_MAX_LEN] = {0};
    snprintf(dbpath, sizeof(dbpath), "%s/%s/%d.kch%s", path, pns->name, new_node->id, args);
    strncpy(new_node->path, dbpath, sizeof(new_node->path));
    if (db_open(new_node->db, new_node->path, new_node->state) < 0) {new_node->state = S_LOST; free(new_node); return -1;}
    pns->number += 1;
    new_node->count = kcdbcount(new_node->db);

    new_node->prev = pns->tail;
    new_node->next = NULL;
    if (pns->tail) {pns->tail->next = new_node;}
    else {pns->head = new_node;}
    pns->tail = new_node;
    return 0;
}

// private function
static int db_open(KCDB * db, char * path, ADFS_NODE_STATE state)
{
    switch (state) {
        case S_READ_ONLY:
            if (!kcdbopen(db, path, KCOREADER)) {return -1;}
            break;
        case S_READ_WRITE:
            if (!kcdbopen(db, path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK)) {return -1;}
            break;
	default:
	    return -1;
    }
    return 0;
}

