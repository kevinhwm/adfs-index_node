#include "an.h"


static int node_create(NodeDBList *this, int id, char *path, int path_len, int state);
static void node_release(NodeDBList *this, int id);
static void node_release_all(NodeDBList *this);
static NodeDB * node_get(NodeDBList *this, int id);


int InitNodeDBList(NodeDBList *this)
{
    if (this)
    {
        memset(this, 0, sizeof(NodeDBList));
        this->create = node_create;
        this->release = node_release;
        this->release_all = node_release_all;
        this->push = node_push;
        this->pop = node_pop;

        this->initialized = NODE_INITIALIZED;
        return ADFS_OK;
    }
    return ADFS_ERROR;
}


static int node_create(NodeDBList *this, int id, char *path, int path_len, int state)
{
    NodeDB * tmp = this->head;
    while (tmp)
    {
        if (tmp->id == id)
            return ADFS_ERROR;
        tmp = tmp->next;
    }

    NodeDB * new_node = (NodeDB *)malloc(sizeof(NodeDB));
    if (new_node == NULL)
        return ADFS_ERROR;

    if (sizeof(new_node->path) <= path_len)
    {
        free(new_node);
        return ADFS_ERROR;
    }

    strncpy(new_node->path, path, sizeof(new_node->path));
    new_node->id = id;
    new_node->state = state;
    new_node->db = kcdbnew();

    if (state == S_READ_ONLY)
    {
        if (!kcdbopen(new_node->db, path, KCOCREATE|KCOREADER))
        {
            free(new_node);
            return ADFS_ERROR;
        }
    }
    else if (state == S_READ_WRITE)
    {
        if (!kcdbopen(new_node->db, path, KCOCREATE|KCOWRITER|KCOTRYLOCK))
        {
            free(new_node);
            return ADFS_ERROR;
        }
    }
    else
    {
        free(new_node);
        return ADFS_ERROR;
    }

    new->pre = this->tail;
    new->next = NULL;

    if (this->tail)     // list is not empty.
        this->tail->next = new_node;
    else                // list is empty, both head and tail are null.
        this-head = new_node;

    this->tail = new_node;   // whenever tail point to new node.
    this->number += 1;
    return ADFS_OK;
}


static void node_release(NodeDBList *this, int id)
{
    NodeDB *p = this->head;
    for (; p; p = p->next)
    {
        if (p->id != id)
            continue;

        // find the id!
        if (p == this->head)
        {
            this->head = p->next;
            this->head->pre = NULL;
        }
        if (p == this->tail)
        {
            this->tail = p->pre;
            this->tail->next = NULL;
        }
        else
        {
            p->pre->next = p->next;
            p->next->pre = p->pre;
        }

        kcdbclose(p->db);
        kcdbdel(p->db);
        free(p);
        this->number -= 1;
        break;
    }

    return ;
}


static void node_release_all(NodeDBList *this)
{
    NodeDB *p = this->tail;
    while (p)
    {
        NodeDB *tmp = p;
        p = p->pre;
        this->tail = p;
        this->tail->next = NULL;
        
        kcdbclose(tmp->db);
        kcdbdel(tmp->db);
        free(tmp);
        this->number -= 1;
    }

    return ;
}


static NodeDB * node_get(NodeDBList *this, int id)
{
    NodeDB * tmp = this->tail;
    while (tmp)
    {
        if (tmp->id == id)
            return tmp;
        tmp = tmp->next;
    }

    return ADFS_ERROR
}

