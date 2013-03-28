/* 
 * huangtao@antiy.com
 */

#include <string.h>
#include <kclangc.h>
#include "an.h"


extern KCDB * index_db;

extern unsigned long kc_apow;
extern unsigned long kc_fbp;
extern unsigned long kc_bnum;
extern unsigned long kc_msiz;


ADFS_RESULT an_init(NodeDBList * pnl, char *db_path, 
        unsigned long mem_size, unsigned long max_file_num, unsigned long max_node_num)
{
    int node_num = count_kch(db_path);
    char cache_mode = 'n';
    int init_mode = 1;

    kc_msiz = mem_size * 1024*1024;

    // index db of ADFS-Node
    char path[1024] = {0};
    if (sprintf(path, "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                db_path, kc_apow, kc_fbp, kc_bnum *40, kc_msiz*10) >= sizeof(path))
    {
        return ADFS_ERROR;
    }
    
    int32_t res = kcdbopen( index_db, path, KCOWRITER|KCOCREATE );
    if ( !res ) 
        return ADFS_ERROR;

    // node db of ADFS-Node
    InitNodeDBList(pnl);

    char node_path[1024] = {0};
    for (int i=1; i <= node_num; i++)
    {
        if (snprintf(node_path, sizeof(node_path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                    db_path, i, kc_apow, kc_fbp, kc_bnum*3, kc_msiz) >= sizeof(node_path))
        {
            return ADFS_ERROR;
        }

        if (i < node_num)
            pnl->create(pnl, i, node_path, strlen(node_path), S_READ_ONLY);
        else
            pnl->create(pnl, i, node_path, strlen(node_path), S_READ_WRITE);
    }

    //get size. if too big, split it
    int64_t file_num = kcdbcount( pnl->tail->db );
    if (file_num < 0)
        return ADFS_ERROR;
    if (file_num >= max_file_num)
        split_db(pnl, max_node_num);

    return ADFS_OK;
}


ADFS_RESULT split_db(NodeDBList * pnl, int max_node_num)
{

    return ADFS_OK;
}


