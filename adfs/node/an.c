/* 
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>
#include <kclangc.h>
#include "an.h"


static NodeDBList adfs_node_list;
static KCDB * index_db = NULL;
static char nodedb_path[1024] = {0};
static unsigned long kc_apow;
static unsigned long kc_fbp;
static unsigned long kc_bnum;
static unsigned long kc_msiz;

////////////////////////////////////////////////////////////////////////////////

static ADFS_RESULT split_db(NodeDBList * pnl);
static int count_kch(char * dir);
static ADFS_RESULT check_kch_name(char * name);

////////////////////////////////////////////////////////////////////////////////


ADFS_RESULT an_init(char *db_path, unsigned long mem_size) 
{
    printf("an_init-0\n");

    NodeDBList * pnl = &adfs_node_list;

    int node_num = count_kch(db_path);
    strncpy(nodedb_path, db_path, sizeof(nodedb_path));
    kc_msiz = mem_size * 1024*1024;

    // index db of ADFS-Node
    char path[1024] = {0};
    if (sprintf(path, "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                nodedb_path, kc_apow, kc_fbp, kc_bnum *40, kc_msiz*10) >= sizeof(path))
    {
        return ADFS_ERROR;
    }
    
    printf("an_init-10\n");

    index_db = kcdbnew();
    int32_t res = kcdbopen( index_db, path, KCOWRITER|KCOCREATE );
    if ( !res ) 
        return ADFS_ERROR;

    // node db of ADFS-Node
    init_nodedb_list(pnl);

    printf("an_init-20\n");

    printf("%d\n", node_num);
    for (int i=1; i <= node_num; i++)
    {
        memset(path, 0, sizeof(path));
        if (snprintf(path, sizeof(path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                    nodedb_path, i, kc_apow, kc_fbp, kc_bnum*3, kc_msiz) >= sizeof(path))
        {
            return ADFS_ERROR;
        }

        if (i < node_num)
        {
            printf("create ro db file\n");
            if (pnl->create(pnl, i, path, strlen(path), S_READ_ONLY) == ADFS_ERROR)
                return ADFS_ERROR;
        }
        else
        {
            printf("create rw db file\n");
            printf("%lu\n", pnl);
            if (pnl->create(pnl, i, path, strlen(path), S_READ_WRITE) == ADFS_ERROR)
                return ADFS_ERROR;
        }
    }

    printf("an_init-30\n");

    return ADFS_OK;
}


void an_exit() 
{
    NodeDBList * pnl = &adfs_node_list;

    kcdbclose(index_db);
    kcdbdel(index_db);
    pnl->release_all(pnl);
    return;
}


ADFS_RESULT an_save(const char *fname, size_t fname_len, const char * fp, size_t fp_len)
{
    // check index

    // save in db
    NodeDBList *pnl = &adfs_node_list;

    NodeDB * node = pnl->tail;
    if (node->number >= MAX_FILE_NUM)
    {
        if (split_db(pnl) == ADFS_ERROR)
            return ADFS_ERROR;

        node = pnl->tail;
    }

    if (kcdbset(node->db, fname, fname_len, fp, fp_len))
    {
        //
    }

    return ADFS_OK;
}


////////////////////////////////////////////////////////////////////////////////
// private
static ADFS_RESULT split_db(NodeDBList * pnl)
{
    NodeDB * last_node = pnl->tail;

    if (pnl->switch_state(pnl, last_node->id, S_READ_ONLY) == ADFS_ERROR)
        return ADFS_ERROR;

    char path[1024] = {0};
    if (snprintf(path, sizeof(path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                nodedb_path, pnl->number, kc_apow, kc_fbp, kc_bnum*3, kc_msiz) >= sizeof(path))
    {
        return ADFS_ERROR;
    }

    return pnl->create(pnl, pnl->number, path, strlen(path), S_READ_WRITE);
}


// private
static int count_kch(char * dir)
{
    int count=0;

    DIR* dirp;
    struct dirent* direntp;

    dirp = opendir( dir );
    if( dirp != NULL ) 
    {
        while ((direntp = readdir(dirp)) != NULL)
        {
            if (check_kch_name(direntp->d_name) == ADFS_OK)
                count++;
        }
        closedir( dirp );

        if (count>0)
            return count;
        else
            return 1;
    }
    return 0;
}

// private
static ADFS_RESULT check_kch_name(char * name)
{
    if (name == NULL)
        return ADFS_ERROR;

    if (strlen(name) > 8)
        return ADFS_ERROR;

    char *pos = strstr(name, ".kch");
    if (pos == NULL)
        return ADFS_ERROR;

    for (int i=0; i < pos-name; i++)
    {
        if (name[i] < '0' || name[i] > '9')
            return ADFS_ERROR;
    }

    if (strcmp(pos, ".kch") != 0)
        return ADFS_ERROR;

    return ADFS_OK;
}

