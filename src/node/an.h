/* adfs node -> an
 *
 * huangtao@antiy.com
 */

#include "adfs.h"
#include <kclangc.h>

#define NODE_MAX_FILE_NUM       100000


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
    char            path[ADFS_MAX_PATH];
    unsigned long   number;
}NodeDB;


typedef struct ANNameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    unsigned long number;

    struct NodeDB * head;
    struct NodeDB * tail;
    struct ANNameSpace * pre;
    struct ANNameSpace * next;

    // functions
    ADFS_RESULT (*create)(struct ANNameSpace *, int, char *, int, ADFS_NODE_STATE);
    void (*release_all)(struct ANNameSpace *);
    struct NodeDB * (*get)(struct ANNameSpace *, int);
    ADFS_RESULT (*switch_state)(struct ANNameSpace *, int, ADFS_NODE_STATE);
}ANNameSpace;


typedef struct ANManager
{
    char path[ADFS_MAX_PATH];
    struct ANNameSpace * head;
    struct ANNameSpace * tail;

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;
}ANManager;


// an_namespace.c
ADFS_RESULT anns_init(ANNameSpace *_this, const char * name_space);

// an_manager.c
ADFS_RESULT anm_init(const char * conf_file, const char * dbpath, unsigned long cache_size);
void anm_exit();
ADFS_RESULT anm_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len);
void anm_get(const char * fname, const char * name_space, void ** ppfile_data, size_t *pfile_size);

// an_function.c
void trim_left_white(char * p);
void trim_right_white(char * p);
ADFS_RESULT get_filename_from_url(char *p);

