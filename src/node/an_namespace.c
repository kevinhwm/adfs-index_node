/* an_namespace.c
 *
 * huangtao@antiy.com
 */

#include <pthread.h>
#include "an_namespace.h"

// list member functions. names begin with "ns_"
static void ns_release(ANNameSpace * _this);
static NodeDB * ns_get(ANNameSpace * _this, int id);
static ADFS_RESULT ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state);
static int ns_needto_split(ANNameSpace * _this);
static ADFS_RESULT ns_split_db(ANNameSpace * _this, const char *path, const char *args);
static void ns_count_add(ANNameSpace * _this);
static void ns_syn(ANNameSpace * _this);

// just functions
static ADFS_RESULT node_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state);
static ADFS_RESULT db_open(KCDB * db, char * path, ADFS_NODE_STATE state);


ADFS_RESULT anns_init(ANNameSpace * _this, const char *name_space)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (_this) {
        memset(_this, 0, sizeof(ANNameSpace));
        _this->release = ns_release;
        _this->get = ns_get;
        _this->create = ns_create;
	_this->needto_split = ns_needto_split;
	_this->split_db = ns_split_db;
	_this->count_add = ns_count_add;
	_this->syn = ns_syn;
	_this->number = 0;
	_this->sign_syn = 0;
	pthread_rwlock_init(&_this->lock, NULL);
        strncpy(_this->name, name_space, sizeof(_this->name));
        res = ADFS_OK;
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

static ADFS_RESULT ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state)
{
    pthread_rwlock_wrlock(&_this->lock);
    ADFS_RESULT res = node_create(_this, path, args, state);
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

static ADFS_RESULT ns_split_db(ANNameSpace * _this, const char *path, const char *args)
{
    pthread_rwlock_wrlock(&_this->lock);
    if (_this->tail->count < NODE_MAX_FILE_NUM) { pthread_rwlock_unlock(&_this->lock); return ADFS_OK; }

    NodeDB *pn = _this->tail;
    kcdbclose(pn->db);
    pn->state = S_READ_ONLY;
    if (db_open(pn->db, pn->path, pn->state) == ADFS_ERROR) { pn->state = S_LOST; pthread_rwlock_unlock(&_this->lock); return ADFS_ERROR; }
    node_create(_this, path, args, S_READ_WRITE);
    pthread_rwlock_unlock(&_this->lock);
    return ADFS_OK;
}

static void ns_count_add(ANNameSpace * _this)
{
    pthread_rwlock_wrlock(&_this->lock);
    _this->tail->count++;
    pthread_rwlock_unlock(&_this->lock);
}

/////////////////////////////////////////////////////////////////////////////////
// not use read-write lock

static ADFS_RESULT node_create(ANNameSpace * pns, const char *path, const char *args, ADFS_NODE_STATE state)
{
    NodeDB * new_node = malloc(sizeof(NodeDB));
    if (new_node == NULL) {return ADFS_ERROR;}
    
    new_node->id = pns->number;
    new_node->state = state;
    new_node->db = kcdbnew();

    char dbpath[ADFS_MAX_PATH] = {0};
    snprintf(dbpath, sizeof(dbpath), "%s/%s/%d.kch%s", path, pns->name, new_node->id, args);
    strncpy(new_node->path, dbpath, sizeof(new_node->path));
    if (db_open(new_node->db, new_node->path, new_node->state) == ADFS_ERROR) {new_node->state = S_LOST; free(new_node); return ADFS_ERROR;}
    pns->number += 1;
    new_node->count = kcdbcount(new_node->db);

    new_node->prev = pns->tail;
    new_node->next = NULL;
    if (pns->tail) {pns->tail->next = new_node;}
    else {pns->head = new_node;}
    pns->tail = new_node;
    return ADFS_OK;
}

// private function
static ADFS_RESULT db_open(KCDB * db, char * path, ADFS_NODE_STATE state)
{
    switch (state) {
        case S_READ_ONLY:
            if (!kcdbopen(db, path, KCOREADER)) {return ADFS_ERROR;}
            break;
        case S_READ_WRITE:
	case S_SYN:
            if (!kcdbopen(db, path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK)) {return ADFS_ERROR;}
            break;
	default:
	    return ADFS_ERROR;
    }
    return ADFS_OK;
}

static void * thread_syn(void *param);
static void ns_syn(ANNameSpace * _this)
{
    // avoid reentrant problem
    pthread_rwlock_wrlock(&_this->lock);
    if (_this->sign_syn) { pthread_rwlock_unlock(&_this->lock); return; }
    _this->sign_syn = 1;
    pthread_rwlock_unlock(&_this->lock);
    pthread_t tid;
    pthread_create(&tid, NULL, thread_syn, NULL);
    return ;
}

static void * thread_syn(void *param)
{
    ANNameSpace *pns = (ANNameSpace *)param;
    NodeDB * pn = pns->head;
    while (pn) {
	pthread_rwlock_wrlock(&pns->lock);
	if (pn->state == S_READ_ONLY) {
	    kcdbclose(pn->db);
	    pn->state = S_SYN;
	    if (db_open(pn->db, pn->path, pn->state) == ADFS_ERROR) {pn->state = S_LOST; pthread_rwlock_unlock(&pns->lock); return NULL;}
	}
	pthread_rwlock_unlock(&pns->lock);

	char *kbuf;
	size_t ksize;
	KCCUR *cur = kcdbcursor(pn->db);
	kccurjump(cur);
	while ((kbuf = kccurgetkey(cur, &ksize, 0)) != NULL) {
	    if (kcdbcheck(pns->index_db, kbuf, ksize) == -1) {kccurremove(cur);}
	    else {kccurstep(cur);}
	    kcfree(kbuf);
	}
	kccurdel(cur);

	if (pn != pns->tail) {
	    pthread_rwlock_wrlock(&pns->lock);
	    if (pn->state == S_READ_WRITE) {
		kcdbclose(pn->db);
		pn->state = S_READ_ONLY;
		if (db_open(pn->db, pn->path, pn->state) == ADFS_ERROR) {pn->state = S_LOST; pthread_rwlock_unlock(&pns->lock); return NULL;}
	    }
	    pthread_rwlock_unlock(&pns->lock);
	}

	pn = pn->next;
    }

    pthread_rwlock_wrlock(&pns->lock);
    pns->sign_syn = 0;
    pthread_rwlock_unlock(&pns->lock);
    return NULL;
}

