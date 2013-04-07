/* an_manager.c
 *
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kclangc.h>
#include "an.h"


static ADFS_RESULT split_db(ANNameSpace * pns);
static int count_kch(const char * dir);
static ADFS_RESULT check_kch_name(char * name);
static ANNameSpace * mgr_get_ns(const char * name_space);

ANManager g_manager;


ADFS_RESULT mgr_init(const char * conf_file, const char *path, unsigned long mem_size) 
{
    memset(&g_manager, 0, sizeof(g_manager));
    if (strlen(path) > MAX_PATH_LENGTH)
        return ADFS_ERROR;

    DIR *dirp = opendir(path);
    if( dirp == NULL ) 
        return ADFS_ERROR;
    closedir(dirp);

    strncpy(g_manager.path, path, sizeof(g_manager.path));
    g_manager.kc_apow = 0;
    g_manager.kc_fbp = 10;
    g_manager.kc_bnum = 1000000;
    g_manager.kc_msiz = mem_size *1024*1024;

    if (mgr_create("default") == NULL)
        return ADFS_ERROR;

    return ADFS_OK;
}

ANNameSpace * mgr_create(const char *name_space) 
{
    printf("mgr-create 0\n");

    if (strlen(name_space) >= MAX_PATH_LENGTH)
        return NULL;

    ANManager *pm = &g_manager;

    char ns_path[MAX_PATH_LENGTH] = {0};
    snprintf(ns_path, sizeof(ns_path), "%s/%s", pm->path, name_space);

    int node_num = count_kch(ns_path);

    // index db of ADFS-Node
    char indexdb_path[1024] = {0};
    if (snprintf(indexdb_path, sizeof(indexdb_path), "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                ns_path, pm->kc_apow, pm->kc_fbp, pm->kc_bnum *40, pm->kc_msiz) >= sizeof(indexdb_path))
    {
        return NULL;
    }
    
    printf("mgr-create 10\n");

    ANNameSpace * pns = malloc(sizeof(ANNameSpace));
    ns_init(pns, name_space);

    pns->index_db = kcdbnew();
    int32_t res = kcdbopen( pns->index_db, indexdb_path, KCOWRITER|KCOCREATE );
    if ( !res ) 
    {
        free(pns);
        return NULL;
    }

    // node db of ADFS-Node
    printf("mgr-create 20\n");

    printf("%d\n", node_num);
    char tmp_path[MAX_PATH_LENGTH] = {0};
    for (int i=1; i <= node_num; i++)
    {
        memset(tmp_path, 0, sizeof(tmp_path));
        if (snprintf(tmp_path, sizeof(tmp_path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                    ns_path, i, pm->kc_apow, pm->kc_fbp, pm->kc_bnum*3, pm->kc_msiz) >= sizeof(tmp_path))
        {
            return NULL;
        }

        if (i < node_num)
        {
            printf("create ro db file\n");
            if (pns->create(pns, i, tmp_path, strlen(tmp_path), S_READ_ONLY) == ADFS_ERROR)
                return NULL;
        }
        else
        {
            printf("create rw db file\n");
            if (pns->create(pns, i, tmp_path, strlen(tmp_path), S_READ_WRITE) == ADFS_ERROR)
                return NULL;
        }
    }

    printf("mgr-create 30\n");
    
    pns->pre = pm->tail;
    pns->next = NULL;
    if (pm->tail)
        pm->tail->next = pns;
    else
        pm->head = pns;
    pm->tail = pns;

    return pns;
}


void mgr_exit() 
{
    ANManager * pm = &g_manager;

    ANNameSpace * pns = pm->tail;
    while (pns)
    {
        ANNameSpace *tmp = pns->pre;
        kcdbclose(pns->index_db);
        kcdbdel(pns->index_db);
        pns->release_all(pns);
        free(pns);
        pns = tmp;
    }
    return;
}


ADFS_RESULT mgr_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len)
{
    printf("mgr-save 0\n");
    if (fp_len > MAX_FILE_SIZE)
        return ADFS_ERROR;

    printf("mgr-save 10\n");
    ANNameSpace * pns = mgr_get_ns(name_space);
    if (pns == NULL)
        if ((pns = mgr_create(name_space)) == NULL)
            return ADFS_ERROR;

    printf("mgr-save 20\n");
    if (pns->index_db == NULL)
        return ADFS_ERROR;

    printf("mgr-save 30\n");
    // check number and split db
    NodeDB * node = pns->tail;
    if (node->number >= MAX_FILE_NUM)
    {
        if (split_db(pns) == ADFS_ERROR)
            return ADFS_ERROR;

        node = pns->tail;
    }

    printf("mgr-save 40\n");
    size_t info_len;
    char * info = kcdbget(pns->index_db, fname, fname_len, &info_len);
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
        if (!kcdbset(pns->index_db, fname, fname_len, buf, strlen(buf)))
            return ADFS_ERROR;
    }

    printf("mgr-save 50\n");
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
    ANManager * pm = &g_manager;
    NodeDB * last_node = pns->tail;

    if (pns->switch_state(pns, last_node->id, S_READ_ONLY) == ADFS_ERROR)
        return ADFS_ERROR;

    char path[1024] = {0};
    if (snprintf(path, sizeof(path), "%s/%lu.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                pm->path, pns->number, pm->kc_apow, pm->kc_fbp, pm->kc_bnum*3, pm->kc_msiz) >= sizeof(path))
    {
        return ADFS_ERROR;
    }

    return pns->create(pns, pns->number, path, strlen(path), S_READ_WRITE);
}


// private
static int count_kch(const char * dir)
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
    else
    {
        mkdir(dir, 0755);
        return 1;
    }
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

// private
static ANNameSpace * mgr_get_ns(const char * name_space)
{
    ANNameSpace * tmp = g_manager.tail;
    while (tmp)
    {
        if (strcmp(tmp->name, name_space) == 0)
            return tmp;
        tmp = tmp->pre;
    }

    return NULL;
}
