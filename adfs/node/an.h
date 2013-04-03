/* adfs node -> an
 *
 * huangtao@antiy.com
 */

#include <kclangc.h>

#define ADFS_VERSION        3.0

typedef int ADFS_RESULT;
#define ADFS_OK             0
#define ADFS_ERROR          -1

typedef int ADFS_NODE_STATE;
#define S_READ_ONLY         0
#define S_READ_WRITE        1

#define MAX_FILE_NUM        100000

#define MAX_FILE_SIZE       0x08000000      // 2^3 * 16^6 = 2^27 = 128MB
#define SPLIT_FILE_SIZE     0x04000000      // 2^2 * 16^6 = 2^26 = 64MB


typedef struct NodeDB
{
    struct NodeDB * pre;
    struct NodeDB * next;

    int             id;
    int             state;
    KCDB        *   db;
    char            path[1024];
    unsigned long   number;
}NodeDB;


#define NODE_INITIALIZED    0x55AA

typedef struct NodeDBList
{
    // data
    struct NodeDB * head;
    struct NodeDB * tail;
    unsigned long   number;

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
ADFS_RESULT an_init(char *dbpath, unsigned long cache_size);
void an_exit();
ADFS_RESULT an_save(const char *fname, size_t fname_len, void * fp, size_t fp_len);
