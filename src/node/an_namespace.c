/* an_namespace.c
 *
 * huangtao@antiy.com
 */

#include "an.h"


// list member functions. names begin with "ns_"
static ADFS_RESULT ns_create(ANNameSpace * _this, int id, char *path, int path_len, ADFS_NODE_STATE state);
static void ns_release(ANNameSpace * _this, int id);
static void ns_release_all(ANNameSpace * _this);
static NodeDB * ns_get(ANNameSpace * _this, int id);
static ADFS_RESULT ns_switch_state(ANNameSpace *_this, int id, ADFS_NODE_STATE des_state);

// just functions
static ADFS_RESULT db_create(KCDB * db, char * path, ADFS_NODE_STATE state);


ADFS_RESULT ns_init(ANNameSpace * _this, const char *name_space)
{
    if (_this)
    {
        memset(_this, 0, sizeof(ANNameSpace));
        _this->create = ns_create;
        _this->release = ns_release;
        _this->release_all = ns_release_all;
        _this->get = ns_get;
        _this->switch_state = ns_switch_state;
        strncpy(_this->name, name_space, sizeof(_this->name));

        return ADFS_OK;
    }
    return ADFS_ERROR;
}


/////////////////////////////////////////////////////////////////////////////////
// public function

// just create, no check.
static ADFS_RESULT ns_create(ANNameSpace * _this, int id, char *path, int path_len, ADFS_NODE_STATE state)
{
    DBG_PRINTS("ns-create 1\n");
    NodeDB * tmp = _this->head;
    while (tmp)
    {
        if (tmp->id == id)
            return ADFS_ERROR;
        tmp = tmp->next;
    }

    DBG_PRINTS("ns-create 2\n");
    NodeDB * new_node = (NodeDB *)malloc(sizeof(NodeDB));
    if (new_node == NULL)
        return ADFS_ERROR;

    if (sizeof(new_node->path) <= path_len)
    {
        free(new_node);
        return ADFS_ERROR;
    }

    DBG_PRINTS("ns-create 3\n");
    strncpy(new_node->path, path, sizeof(new_node->path));
    new_node->number = 0;
    new_node->id = id;
    new_node->state = state;
    new_node->db = kcdbnew();

    DBG_PRINTS("ns-create 4\n");
    DBG_PRINTS("path: ");
    DBG_PRINTSN(path);
    if (db_create(new_node->db, path, state) == ADFS_ERROR)
    {
        free(new_node);
        return ADFS_ERROR;
    }

    DBG_PRINTS("ns-create 5\n");
    new_node->pre = _this->tail;
    new_node->next = NULL;

    DBG_PRINTS("ns-create 6\n");
    if (_this->tail)            // list is not empty.
        _this->tail->next = new_node;
    else                        // list is empty, both head and tail are null.
        _this->head = new_node;
    DBG_PRINTS("ns-create 7\n");
    _this->tail = new_node;     // whenever tail point to new node.
    _this->number += 1;

    DBG_PRINTS("ns-create 8\n");
    return ADFS_OK;
}


static void ns_release(ANNameSpace * _this, int id)
{
    NodeDB *p = _this->head;
    for (; p; p = p->next)
    {
        if (p->id != id)
            continue;

        // find the id!
        if (p == _this->head)
        {
            _this->head = p->next;
            _this->head->pre = NULL;
        }
        else if (p == _this->tail)
        {
            _this->tail = p->pre;
            _this->tail->next = NULL;
        }
        else
        {
            p->pre->next = p->next;
            p->next->pre = p->pre;
        }

        kcdbclose(p->db);
        kcdbdel(p->db);
        free(p);
        _this->number -= 1;

        break;
    }

    return ;
}


static void ns_release_all(ANNameSpace * _this)
{
    while (_this->tail)
    {
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
    NodeDB * tmp = _this->tail;
    while (tmp)
    {
        if (tmp->id == id)
            return tmp;
        tmp = tmp->next;
    }

    return NULL;
}


static ADFS_RESULT ns_switch_state(ANNameSpace * _this, int id, ADFS_NODE_STATE des_state)
{
    NodeDB *tmp = _this->get(_this, id);
    if (tmp == NULL)
        return ADFS_ERROR;

    kcdbclose(tmp->db);
    tmp->state = des_state;

    return db_create(tmp->db, tmp->path, tmp->state);
}


/////////////////////////////////////////////////////////////////////////////////
// private function
static ADFS_RESULT db_create(KCDB * db, char * path, ADFS_NODE_STATE state)
{
    switch (state)
    {
        case S_READ_ONLY:
            if (!kcdbopen(db, path, KCOREADER))
                return ADFS_ERROR;
            break;

        case S_READ_WRITE:
            if (!kcdbopen(db, path, KCOCREATE|KCOWRITER|KCOTRYLOCK))
                return ADFS_ERROR;
            break;
    }

    return ADFS_OK;
}

