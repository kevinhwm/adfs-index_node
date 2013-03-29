/* adfs node -> an
 *
 * huangtao@antiy.com
 */

#include <kclangc.h>


typedef int ADFS_RESULT;
#define ADFS_OK             0
#define ADFS_ERROR          -1

typedef int ADFS_NODE_STATE;
#define S_READ_ONLY         0
#define S_READ_WRITE        1


typedef struct NodeDB
{
    struct NodeDB * pre;
    struct NodeDB * next;

    int     id;
    int     state;
    KCDB *  db;
    char    path[1024];
}NodeDB;


#define NODE_INITIALIZED        0x55AA
typedef struct NodeDBList
{
    // data
    struct NodeDB * head;
    struct NodeDB * tail;
    long number;

    // functions
    ADFS_RESULT (*create)(struct NodeDBList *, int, char *, int, ADFS_NODE_STATE);
    void (*release)(struct NodeDBList *, int);
    void (*release_all)(struct NodeDBList *);
    struct NodeDB * (*get)(struct NodeDBList *, int);
    ADFS_RESULT (*switch_state)(struct NodeDBList *, int, ADFS_NODE_STATE);

    // private
    long initialized;
}NodeDBList;


// an_list.c
ADFS_RESULT init_nodedb_list(NodeDBList *_this);

// an.c
ADFS_RESULT an_init(NodeDBList * node_list, char *dbpath, unsigned long cache_size, 
        unsigned long kchfile_size, unsigned long max_node_count);

void an_exit(NodeDBList * node_list);

ADFS_RESULT split_db(NodeDBList * node_list, int max_node_num);

