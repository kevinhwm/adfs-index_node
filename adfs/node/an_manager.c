/* an_manager.c
 *
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>
#include <kclangc.h>
#include "an.h"


static ADFS_RESULT mgr_scan(char *db_path, unsigned long mem_size);

static ADFS_RESULT split_db(ANNameSpace * pns);
static int count_kch(char * dir);
static ADFS_RESULT check_kch_name(char * name);

ANManager g_manager;

ADFS_RESULT mgr_init(char *path, unsigned long mem_size) 
{
    memset(&g_manager, 0, sizeof(g_manager));
    if (strlen(path) > MAX_PATH_LENGTH)
        return ADFS_ERROR;

    DIR *dirp = opendir(path);
    if( dirp == NULL ) 
        return ADFS_ERROR;
    closedir(dirp);

    strncpy(g_manager.path, path, sizeof(g_manager.path));
    g_manager.kc_apw = 
    g_manager.kc_fbp = 
    g_manager.kc_bnum = 
    g_manager.kc_msiz = mem_size
    g_manager.scan = mgr_scan;

    return ADFS_OK;
}

static ADFS_RESULT mgr_scan(char *db_path, unsigned long mem_size) 
{
    printf("an_init-0\n");

    ANNameSpace * pns = malloc(sizeof(ANNameSpace));

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
    ns_init(pns);

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
            if (pns->create(pns, i, path, strlen(path), S_READ_ONLY) == ADFS_ERROR)
                return ADFS_ERROR;
        }
        else
        {
            printf("create rw db file\n");
            printf("%lu\n", pns);
            if (pns->create(pns, i, path, strlen(path), S_READ_WRITE) == ADFS_ERROR)
                return ADFS_ERROR;
        }
    }

    printf("an_init-30\n");

    return ADFS_OK;
}


void mgr_exit() 
{
    ANNameSpace * pns = &adfs_node_list;

    kcdbclose(index_db);
    kcdbdel(index_db);
    pns->release_all(pns);
    return;
}


ADFS_RESULT mgr_save(const char *fname, size_t fname_len, void * fp, size_t fp_len)
{
    if (fp_len > MAX_FILE_SIZE)
        return ADFS_ERROR;

    if (index_db == NULL)
        return ADFS_ERROR;

    // check number and split db
    ANNameSpace *pns = &adfs_node_list;
    NodeDB * node = pns->tail;
    if (node->number >= MAX_FILE_NUM)
    {
        if (split_db(pns) == ADFS_ERROR)
            return ADFS_ERROR;

        node = pns->tail;
    }

    size_t info_len;
    char * info = kcdbget(index_db, fname, fname_len, &info_len);
    if (info != NULL)
    {
        NodeDB * tmp = pns->get(pns, atoi(info));
        kcfree(info);
        if (tmp == NULL)
            return ADFS_ERROR;
        // save into db
        if (!kcdbset(tmp->db, fname, fname_len, fp, fp_len))
            return ADFS_ERROR;
    }
    else
    {
        // save into db
        if (!kcdbset(pns->tail->db, fname, fname_len, fp, fp_len))
            return ADFS_ERROR;

        // save into index
        char buf[16] = {0};
        sprintf(buf, "%d", pns->tail->id);
        if (!kcdbset(index_db, fname, fname_len, buf, strlen(buf)))
            return ADFS_ERROR;
    }

    return ADFS_OK;
}


void mgr_get_file(const char * fname, const char * name_space, void ** ppfile_data, size_t *pfile_size)
{

    return ;
}


////////////////////////////////////////////////////////////////////////////////
// private
static ADFS_RESULT split_db(ANNameSpace * pns)
{
    NodeDB * last_node = pns->tail;

    if (pns->switch_state(pns, last_node->id, S_READ_ONLY) == ADFS_ERROR)
        return ADFS_ERROR;

    char path[1024] = {0};
    if (snprintf(path, sizeof(path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                nodedb_path, pns->number, kc_apow, kc_fbp, kc_bnum*3, kc_msiz) >= sizeof(path))
    {
        return ADFS_ERROR;
    }

    return pns->create(pns, pns->number, path, strlen(path), S_READ_WRITE);
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

