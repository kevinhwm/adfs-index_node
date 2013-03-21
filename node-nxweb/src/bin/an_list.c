#include "an.h"


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
    NodeDB * p = (NodeDB *)malloc(sizeof(NodeDB));
    if (p == NULL)
        return ADFS_ERROR;

    if (sizeof(p->path) <= path_len)
    {
        free(p);
        return ADFS_ERROR;
    }

    strncpy(p->path, path, sizeof(p->path));
    p->id = id;
    p->state = state;
    p->db = kcdbnew();

    p->pre = this->tail;
    p->next = NULL;

    if (this->tail)     // list is not empty.
        this->tail->next = p;
    else                // list is empty, both head and tail are null.
        this-head = p;

    this->tail = p;     // whenever tail point to new node.
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

        p->pre->next = p->next;
        p->next->pre = p->pre;
        if (p == this->head)
            this->head = p->next;
        if (p == this->tail)
            this->tail = p->pre;

        kcdbclose(p->db);
        kcdbdel(p->db);
        free(p);
        this->number -= 1;
        break;
    }

    return ;
}

static void node_release_all(NodeDBList)
{
    NodeDB *p = this->tail;
    while (p)
    {
        NodeDB *tmp = p;
        p = p->pre;
        p->next = NULL;
        this->tail = p;
        
        kcdbclose(tmp->db);
        kcdbdel(tmp->db);
        free(tmp);
        this->number -= 1;
    }

    return ;
}

int node_push()
{}

int node_pop()
{}

NodeDB * node_get()
{}

