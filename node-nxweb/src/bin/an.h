/*  adfs node -> an
 *
 */

#define ADFS_OK             0
#define ADFS_ERROR          -1

#define S_READ_ONLY         0
#define S_READ_WRITE        1


typedef struct NodeDB
{
    NodeDB * pre;
    NodeDB * next;

    int     id;
    int     state;
    KCDB *  db;
    char    path[1024];
}NodeDB;

#define NODE_INITIALIZED        0x55AA
typedef struct NodeDBList
{
    // data
    NodeDB * head;
    NodeDB * tail;
    long number;
    long initialized;

    // functions
    int (*create)(int, char *, int);
    void (*release)();
    void (*release_all)();
    int (*push)();
    int (*pop)();
    NodeDB * (*get)(int);
}
NodeDBList;


