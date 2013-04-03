/* adfs node -> an
 *
 * huangtao@antiy.com
 */

#include <kclangc.h>

#define ADFS_VERSION        3.0

#define MAX_FILE_NUM        100000
#define MAX_URL_LENGTH      2048
#define MAX_PATH_LENGTH     1024

#define MAX_FILE_SIZE       0x08000000      // 2^3 * 16^6 = 2^27 = 128MB
#define SPLIT_FILE_SIZE     0x04000000      // 2^2 * 16^6 = 2^26 = 64MB


typedef enum ADFS_RESULT
{
    ADFS_OK         = 0,
    ADFS_ERROR      = -1,
}ADFS_RESULT;

typedef enum ADFS_NODE_STATE
{
    S_READ_ONLY     = 0,
    S_READ_WRITE    = 1,
}ADFS_NODE_STATE;


typedef struct NodeDB
{
    struct NodeDB * pre;
    struct NodeDB * next;

    int             id;
    ADFS_NODE_STATE state;
    KCDB        *   db;
    char            path[1024];
    unsigned long   number;
}NodeDB;


typedef struct ANNameSpace
{
    // data
    struct ANNameSpace * pre;
    struct ANNameSpace * next;
    struct NodeDB * head;
    struct NodeDB * tail;

    char name[MAX_PATH_LENGTH];
    KCDB * index_db;
    unsigned long number;

    // functions
    ADFS_RESULT (*create)(struct ANNameSpace *, int, char *, int, ADFS_NODE_STATE);
    void (*release)(struct ANNameSpace *, int);
    void (*release_all)(struct ANNameSpace *);
    struct NodeDB * (*get)(struct ANNameSpace *, int);
    ADFS_RESULT (*switch_state)(struct ANNameSpace *, int, ADFS_NODE_STATE);
}ANNameSpace;


typedef struct ANManager
{
    char path[MAX_PATH_LENGTH];
    struct ANNameSpace * head;
    struct ANNameSpace * tail;

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

}ANManager;


// an_namespace.c
ADFS_RESULT ns_init(ANNameSpace *_this, const char * name_space);

// an_manager.c
ADFS_RESULT mgr_init(const char * dbpath, unsigned long cache_size);
ANNameSpace * mgr_create(const char *name_space);
void mgr_exit();
ADFS_RESULT mgr_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len);

