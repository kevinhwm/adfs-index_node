/* an_namespace.c
 *
 * huangtao@antiy.com
 */

#include "an_namespace.h"

// list member functions. names begin with "ns_"
static ADFS_RESULT ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state);
static void ns_release_all(ANNameSpace * _this);
static NodeDB * ns_get(ANNameSpace * _this, int id);
static ADFS_RESULT ns_switch_state(ANNameSpace *_this, int id, ADFS_NODE_STATE des_state);

// just functions
static ADFS_RESULT db_create(KCDB * db, char * path, ADFS_NODE_STATE state);


ADFS_RESULT anns_init(ANNameSpace * _this, const char *name_space)
{
    if (_this) {
        memset(_this, 0, sizeof(ANNameSpace));
        _this->create = ns_create;
        _this->release_all = ns_release_all;
        _this->get = ns_get;
	_this->number = 0;
        _this->switch_state = ns_switch_state;
        strncpy(_this->name, name_space, sizeof(_this->name));
        return ADFS_OK;
    }
    return ADFS_ERROR;
}

// just create, no check.
static ADFS_RESULT ns_create(ANNameSpace * _this, const char *path, const char *args, ADFS_NODE_STATE state)
{
    NodeDB * new_node = (NodeDB *)malloc(sizeof(NodeDB));
    if (new_node == NULL)
        return ADFS_ERROR;

    new_node->id = _this->number;
    new_node->state = state;
    new_node->db = kcdbnew();

    char dbpath[ADFS_MAX_PATH] = {0};
    snprintf(dbpath, sizeof(dbpath), "%s/%s/%d.kch%s", path, _this->name, new_node->id, args);
    if (db_create(new_node->db, dbpath, state) == ADFS_ERROR)
	goto err1;
    strncpy(new_node->path, dbpath, sizeof(new_node->path));
    _this->number += 1;
    new_node->count = kcdbcount(new_node->db);
    new_node->count = new_node->count < 0 ? 0 : new_node->count;

    new_node->pre = _this->tail;
    new_node->next = NULL;
    if (_this->tail)
        _this->tail->next = new_node;
    else
        _this->head = new_node;
    _this->tail = new_node;

    return ADFS_OK;
err1:
    free(new_node);
    return ADFS_ERROR;
}

static void ns_release_all(ANNameSpace * _this)
{
    while (_this->tail) {
        NodeDB *tmp = _this->tail;
        _this->tail = _this->tail->pre;
        kcdbclose(tmp->db);
        kcdbdel(tmp->db);
        free(tmp);
    }
    return ;
}

static NodeDB * ns_get(ANNameSpace * _this, int id)
{
    NodeDB * pn = _this->head;
    while (pn) {
        if (pn->id == id)
            return pn;
        pn = pn->next;
    }
    return NULL;
}

static ADFS_RESULT ns_switch_state(ANNameSpace * _this, int id, ADFS_NODE_STATE des_state)
{
    DBG_PRINTSN("switch 10");
    DBG_PRINTIN(id);
    NodeDB *pn = _this->get(_this, id);
    if (pn == NULL)
        return ADFS_ERROR;
    DBG_PRINTSN("switch 20");
    kcdbclose(pn->db);
    pn->state = des_state;
    return db_create(pn->db, pn->path, pn->state);
}

/////////////////////////////////////////////////////////////////////////////////
// private function
static ADFS_RESULT db_create(KCDB * db, char * path, ADFS_NODE_STATE state)
{
    switch (state) {
        case S_READ_ONLY:
            if (!kcdbopen(db, path, KCOREADER))
                return ADFS_ERROR;
            break;
        case S_READ_WRITE:
            if (!kcdbopen(db, path, KCOREADER|KCOCREATE|KCOWRITER|KCOTRYLOCK))
                return ADFS_ERROR;
            break;
    }
    return ADFS_OK;
}

