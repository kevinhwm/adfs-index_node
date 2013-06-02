/* an_namespace.c
 *
 * huangtao@antiy.com
 */

#include <pthread.h>
#include "an_namespace.h"

// list member functions. names begin with "ns_"
static void ns_release_all(ANNameSpace * _this);
static ADFS_RESULT ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state);
static NodeDB * ns_get(ANNameSpace * _this, int id);
static int ns_needto_split(ANNameSpace * _this);
static ADFS_RESULT ns_split_db(ANNameSpace * _this, const char *path, const char *args);
static void ns_count_add(ANNameSpace * _this);

// just functions
static ADFS_RESULT db_create(KCDB * db, char * path, ADFS_NODE_STATE state);


ADFS_RESULT anns_init(ANNameSpace * _this, const char *name_space)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (_this) {
        memset(_this, 0, sizeof(ANNameSpace));
        _this->release_all = ns_release_all;
        _this->create = ns_create;
        _this->get = ns_get;
	_this->needto_split = ns_needto_split;
	_this->split_db = ns_split_db;
	_this->count_add = ns_count_add;
	_this->number = 0;
	pthread_rwlock_init(&_this->lock, NULL);
        strncpy(_this->name, name_space, sizeof(_this->name));
        res = ADFS_OK;
    }
    return res;
}

static void ns_release_all(ANNameSpace * _this)
{
    pthread_rwlock_destroy(&_this->lock);
    while (_this->tail) {
        NodeDB *tmp = _this->tail;
        _this->tail = _this->tail->pre;
        kcdbclose(tmp->db);
        kcdbdel(tmp->db);
        free(tmp);
    }
    return ;
}

// just create, no check.
static ADFS_RESULT ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state)
{
    NodeDB * new_node = malloc(sizeof(NodeDB));
    if (new_node == NULL) {return ADFS_ERROR;}
    
    new_node->id = _this->number;
    new_node->state = state;
    new_node->db = kcdbnew();

    char dbpath[ADFS_MAX_PATH] = {0};
    snprintf(dbpath, sizeof(dbpath), "%s/%s/%d.kch%s", path, _this->name, new_node->id, args);
    if (db_create(new_node->db, dbpath, state) == ADFS_ERROR) {goto err1;}
    strncpy(new_node->path, dbpath, sizeof(new_node->path));
    _this->number += 1;
    new_node->count = kcdbcount(new_node->db);
    new_node->count = new_node->count < 0 ? 0 : new_node->count;

    new_node->pre = _this->tail;
    new_node->next = NULL;
    if (_this->tail) {_this->tail->next = new_node;}
    else {_this->head = new_node;}
    _this->tail = new_node;
    return ADFS_OK;
err1:
    free(new_node);
    return ADFS_ERROR;
}

static NodeDB * ns_get(ANNameSpace * _this, int id)
{
    pthread_rwlock_rdlock(&_this->lock);
    DBG_PRINTSN("lock ns_get");
    NodeDB * pn = _this->head;
    for (; pn; pn = pn->next) {
        if (pn->id == id) {
	    pthread_rwlock_unlock(&_this->lock);
	    DBG_PRINTSN("unlock ns_get");
	    return pn;
	}
    }
    pthread_rwlock_unlock(&_this->lock);
    DBG_PRINTSN("unlock ns_get");
    return NULL;
}

static int ns_needto_split(ANNameSpace * _this)
{
    pthread_rwlock_rdlock(&_this->lock);
    DBG_PRINTSN("lock ns_needto_split");
    int res = _this->tail->count >= NODE_MAX_FILE_NUM;
    pthread_rwlock_unlock(&_this->lock);
    DBG_PRINTSN("unlock ns_needto_split");
    return res;
}

static ADFS_RESULT ns_split_db(ANNameSpace * _this, const char *path, const char *args)
{
    pthread_rwlock_wrlock(&_this->lock);
    DBG_PRINTSN("lock ns_split_db");
    if (_this->tail->count < NODE_MAX_FILE_NUM) {
	pthread_rwlock_unlock(&_this->lock);
	DBG_PRINTSN("unlock ns_split_db");
	return ADFS_OK;
    }

    NodeDB *pn = _this->tail;
    kcdbclose(pn->db);
    pn->state = S_READ_ONLY;
    if (db_create(pn->db, pn->path, pn->state) == ADFS_ERROR) {
	pthread_rwlock_unlock(&_this->lock);
	DBG_PRINTSN("unlock ns_split_db. create db error");
	return ADFS_ERROR;
    }
    _this->create(_this, path, args, S_READ_WRITE);
    pthread_rwlock_unlock(&_this->lock);
    DBG_PRINTSN("unlock ns_split_db");
    return ADFS_OK;
}

static void ns_count_add(ANNameSpace * _this)
{
    pthread_rwlock_wrlock(&_this->lock);
    DBG_PRINTSN("lock ns_count_add");
    _this->tail->count++;
    pthread_rwlock_unlock(&_this->lock);
    DBG_PRINTSN("unlock ns_count_add");
}

/////////////////////////////////////////////////////////////////////////////////
// private function
static ADFS_RESULT db_create(KCDB * db, char * path, ADFS_NODE_STATE state)
{
    switch (state) {
        case S_READ_ONLY:
            if (!kcdbopen(db, path, KCOREADER)) {return ADFS_ERROR;}
            break;
        case S_READ_WRITE:
            if (!kcdbopen(db, path, KCOREADER|KCOCREATE|KCOWRITER|KCOTRYLOCK)) {return ADFS_ERROR;}
            break;
    }
    return ADFS_OK;
}

