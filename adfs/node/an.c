/* 
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>
#include <kclangc.h>
#include "an.h"


extern KCDB * index_db;

extern char nodedb_path[1024];
extern unsigned long kc_apow;
extern unsigned long kc_fbp;
extern unsigned long kc_bnum;
extern unsigned long kc_msiz;

////////////////////////////////////////////////////////////////////////////////

static int count_kch(char * dir);
static ADFS_RESULT check_kch_name(char * name);

////////////////////////////////////////////////////////////////////////////////


ADFS_RESULT an_init(NodeDBList * pnl, char *db_path, 
        unsigned long mem_size, unsigned long max_file_num, unsigned long max_node_num)
{
    printf("an_init-0\n");

    int node_num = count_kch(db_path);
    char cache_mode = 'n';
    int init_mode = 1;

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
            if (pnl->create(pnl, i, path, strlen(path), S_READ_ONLY) == ADFS_ERROR)
                return ADFS_ERROR;
        }
        else
        {
            if (pnl->create(pnl, i, path, strlen(path), S_READ_WRITE) == ADFS_ERROR)
                return ADFS_ERROR;
        }
    }

    printf("an_init-30\n");

    //get size. if too big, split it
    int64_t file_num = kcdbcount( pnl->tail->db );
    if (file_num < 0)
        return ADFS_ERROR;
    if (file_num >= max_file_num)
        split_db(pnl, max_node_num);

    printf("an_init-40\n");

    return ADFS_OK;
}


void an_exit(NodeDBList * node_list)
{
    kcdbclose(index_db);
    kcdbdel(index_db);
    node_list->release_all(node_list);
    return;
}


ADFS_RESULT split_db(NodeDBList * pnl, int max_node_num)
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


////////////////////////////////////////////////////////////////////////////////

static int count_kch(char * dir)
{
    int total_count=0;

    DIR* dirp;
    struct dirent* direntp;

    dirp = opendir( dir );
    if( dirp != NULL ) 
    {
        while ((direntp = readdir(dirp)) != NULL)
        {
            if (check_kch_name(direntp->d_name) == ADFS_OK)
                total_count++;
        }

        closedir( dirp );
        if (total_count>0)
            return total_count;
        else
            return 1;
    }
    return 0;
}


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


