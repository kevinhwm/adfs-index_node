/* namespace.c
 *
 * kevinhwm@gmail.com
 */

#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include "namespace.h"

// list member functions. names begin with "ns_"
static void ns_release(CNNameSpace * _this);
static NodeDB * ns_get(CNNameSpace * _this, int id);

static int ns_create(CNNameSpace * _this, const char *path, const char *args, _DFS_NODE_STATE state);
static int ns_needto_split(CNNameSpace * _this);
static int ns_split_db(CNNameSpace * _this, const char *path, const char *args);
static void ns_count_add(CNNameSpace * _this);

// just functions
static int node_create(CNNameSpace * _this, const char *path, const char *args, _DFS_NODE_STATE state);	// called by ns_create(), ns_split_db()
static int db_open(KCDB * db, char * path, _DFS_NODE_STATE state);	// called by node_create(), ns_split_db()
static int scan_kch(const char * dir);					// initialize
static int get_fileid(char * name);					// initialize	called by scan_kch()


int GNns_init(CNNameSpace *_this, const char *name_space, const char *data_dir, const char *db_args)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CNNameSpace));

    _this->release = ns_release;
    _this->create = ns_create;
    _this->get = ns_get;
    _this->needto_split = ns_needto_split;
    _this->split_db = ns_split_db;
    _this->count_add = ns_count_add;

    pthread_rwlock_init(&_this->lock, NULL);
    strncpy(_this->name, name_space, sizeof(_this->name));
    _this->number = 0;
    _this->index_db = kcdbnew();

    char ns_path[_DFS_MAX_LEN] = {0};
    snprintf(ns_path, sizeof(ns_path), "%s/%s", data_dir, name_space);
    int max_id = scan_kch(ns_path); 
    if (max_id < 0) { return -1; }

    char indexdb_path[_DFS_MAX_LEN] = {0};
    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s/index.kch%s", data_dir, name_space, db_args);
    if ( !kcdbopen(_this->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK) ) { return -1; }

    for (int i=0; i <= max_id; ++i) {
	if (i < max_id) {
	    if (_this->create(_this, data_dir, db_args, S_READ_ONLY) < 0) { return -1; }
	}
	else {
	    if (_this->create(_this, data_dir, db_args, S_READ_WRITE) < 0) { return -1; }
	}
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// use read-write lock
static void ns_release(CNNameSpace * _this)
{
    pthread_rwlock_wrlock(&_this->lock);
    if (_this->index_db) {
	kcdbclose(_this->index_db);
	kcdbdel(_this->index_db);
	_this->index_db = NULL;
    }
    while (_this->tail) {
        NodeDB *tmp = _this->tail;
        _this->tail = _this->tail->prev;
	if (tmp->db) {
	    kcdbclose(tmp->db);
	    kcdbdel(tmp->db);
	    tmp->db = NULL;
	}
        free(tmp);
    }
    pthread_rwlock_unlock(&_this->lock);
    pthread_rwlock_destroy(&_this->lock);
    return ;
}

static NodeDB * ns_get(CNNameSpace * _this, int id)
{
    pthread_rwlock_rdlock(&_this->lock);
    NodeDB *pn = _this->head;
    for (;pn; pn = pn->next) {
        if (pn->id == id) { pthread_rwlock_unlock(&_this->lock); return pn; }
    }
    pthread_rwlock_unlock(&_this->lock);
    return NULL;
}

static int ns_create(CNNameSpace * _this, const char *path, const char *args, _DFS_NODE_STATE state)
{
    pthread_rwlock_wrlock(&_this->lock);
    int res = node_create(_this, path, args, state);
    pthread_rwlock_unlock(&_this->lock);
    return res;
}

static int ns_needto_split(CNNameSpace * _this)
{
    pthread_rwlock_rdlock(&_this->lock);
    int res = _this->tail->count >= NODE_MAX_FILE_NUM;
    pthread_rwlock_unlock(&_this->lock);
    return res;
}

static int ns_split_db(CNNameSpace * _this, const char *path, const char *args)
{
    pthread_rwlock_wrlock(&_this->lock);
    if (_this->tail->count < NODE_MAX_FILE_NUM) { 
	pthread_rwlock_unlock(&_this->lock); 
	return 0; 
    }

    NodeDB *pn = _this->tail;
    kcdbclose(pn->db);
    pn->state = S_READ_ONLY;
    if (db_open(pn->db, pn->path, pn->state) < 0) { 
	pn->state = S_NA; 
	pthread_rwlock_unlock(&_this->lock); 
	return -1; 
    }

    node_create(_this, path, args, S_READ_WRITE);
    pthread_rwlock_unlock(&_this->lock);
    return 0;
}

static void ns_count_add(CNNameSpace * _this)
{
    pthread_rwlock_wrlock(&_this->lock);
    _this->tail->count++;
    pthread_rwlock_unlock(&_this->lock);
}

/////////////////////////////////////////////////////////////////////////////////
// not use read-write lock

static int node_create(CNNameSpace * pns, const char *path, const char *args, _DFS_NODE_STATE state)
{
    NodeDB * new_node = malloc(sizeof(NodeDB));
    if (new_node == NULL) { return -1; }
    
    new_node->id = pns->number;
    new_node->state = state;
    new_node->db = kcdbnew();

    char dbpath[_DFS_MAX_LEN] = {0};
    snprintf(dbpath, sizeof(dbpath), "%s/%s/%d.kch%s", path, pns->name, new_node->id, args);
    strncpy(new_node->path, dbpath, sizeof(new_node->path));
    if (db_open(new_node->db, new_node->path, new_node->state) < 0) {
	new_node->state = S_NA; 
	free(new_node); 
	return -1;
    }
    pns->number += 1;
    new_node->count = kcdbcount(new_node->db);

    new_node->prev = pns->tail;
    new_node->next = NULL;
    if (pns->tail) {pns->tail->next = new_node;}
    else {pns->head = new_node;}
    pns->tail = new_node;
    return 0;
}

static int db_open(KCDB * db, char * path, _DFS_NODE_STATE state)
{
    switch (state) {
        case S_READ_ONLY:
            if (!kcdbopen(db, path, KCOREADER)) { return -1; }
            break;
        case S_READ_WRITE:
            if (!kcdbopen(db, path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK)) { return -1; }
            break;
	default:
	    return -1;
    }
    return 0;
}

static int scan_kch(const char * dir)
{
    int max_id = 0;
    DIR* dp;
    struct dirent* dirp;

    dp = opendir(dir);
    if (dp != NULL) {
	while ((dirp = readdir(dp)) != NULL) {
	    int id = get_fileid(dirp->d_name);
	    max_id = id > max_id ? id : max_id;
	}
	closedir(dp);
    }
    else {
	if (mkdir(dir, 0755) < 0) { return -1; }
    }
    return max_id;
}

static int get_fileid(char * name)
{
    if (name == NULL) { return -1; }
    if (strlen(name) > _DFS_NODE_ID_LEN) { return -1; }
    char *pos = strstr(name, ".kch");
    if (pos == NULL) { return -1; }
    if (strcmp(pos, ".kch") != 0) { return -1; }

    char tmp[_DFS_NODE_ID_LEN] = {0};
    strncpy(tmp, name, pos-name);

    for (unsigned int i=0; i<strlen(tmp); ++i) {
	if ( !isdigit(name[i]) ) { return -1; }
    }
    return atoi(tmp);
}

