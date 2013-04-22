/* Antiy Labs. Basic Platform R & D Center.
 * an_manager.c
 *
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kclangc.h>
#include "an.h"


static ANNameSpace * create_ns(const char *name_space);
static ADFS_RESULT split_db(ANNameSpace * pns);
static int count_kch(const char * dir);
static ADFS_RESULT check_kch_name(char * name);
static ANNameSpace * get_ns(const char * name_space);

ANManager g_manager;


ADFS_RESULT mgr_init(const char * conf_file, const char *path, unsigned long mem_size) 
{
    memset(&g_manager, 0, sizeof(g_manager));
    if (strlen(path) > ADFS_MAX_PATH)
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

    // create default namespace
    if (create_ns("default") == NULL)
        return ADFS_ERROR;

    // read config file, create other namespaces
    char buf[ADFS_MAX_PATH] = {0};
    if ( !(get_conf(conf_file, "namespace_num", buf, sizeof(buf))) )
        return ADFS_ERROR;
    int num = atoi(buf);
    for (int i=0; i<num; ++i)
    {
        char key[64] = {0};
        sprintf(key, "namespace_%d", i);
        get_conf(conf_file, key, buf, sizeof(buf));
        if (create_ns(buf) == NULL)
            return ADFS_ERROR;
    }

    return ADFS_OK;
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
}


ADFS_RESULT mgr_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len)
{
    if (fp_len > ADFS_MAX_FILE_SIZE)
        return ADFS_ERROR;

    ANNameSpace * pns = NULL;
    if (name_space == NULL)
        pns = get_ns("default");
    else
        pns = get_ns(name_space);

    if (pns == NULL)
        return ADFS_ERROR;

    if (pns->index_db == NULL)
        return ADFS_ERROR;

    // check number and split db
    NodeDB * node = pns->tail;
    if (node->number >= NODE_MAX_FILE_NUM)
    {
        if (split_db(pns) == ADFS_ERROR)
            return ADFS_ERROR;

        node = pns->tail;
    }

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

    return ADFS_OK;
}


void mgr_get(const char * fname, const char * name_space, void ** ppfile_data, size_t *pfile_size)
{
    *ppfile_data = NULL;
    *pfile_size = 0;

    ANNameSpace * pns = NULL;
    if (name_space == NULL)
        pns = get_ns("default");
    else
        pns = get_ns(name_space);

    if (pns == NULL)
        return ;

    if (pns->index_db == NULL)
        return ;

    size_t len = 0;
    char *id = kcdbget(pns->index_db, fname, strlen(fname), &len);
    if (id == NULL)
        return;

    DBG_PRINTS("mgr-download fname: ");
    DBG_PRINTSN(fname);
    DBG_PRINTS("mgr-download id: ");
    DBG_PRINTSN(id);

    NodeDB * pn = pns->get(pns, atoi(id));
    if (pn == NULL)
        return;

    DBG_PRINTSN("mgr-download 60");
    *ppfile_data = kcdbget(pn->db, fname, strlen(fname), pfile_size);

    DBG_PRINTS("mgr-download 70");
    kcfree(id);
    return ;
}


ADFS_RESULT mgr_delete(const char *fname, const char *name_space)
{
    ANNameSpace * pns = NULL;
    if (name_space == NULL)
        pns = get_ns("default");
    else
        pns = get_ns(name_space);

    if (pns == NULL)
        return ADFS_ERROR;

    if (pns->index_db == NULL)
        return ADFS_ERROR;

    size_t len = 0;
    char *id = kcdbget(pns->index_db, fname, strlen(fname), &len);
    if (id == NULL)
        return;

    NodeDB * pn = pns->get(pns, atoi(id));
    if (pn == NULL)
        return ADFS_ERROR;

    *ppfile_data = kcdbget(pn->db, fname, strlen(fname), pfile_size);

    kcfree(id);


    char key[ADFS_MAX_PATH] = {0};
    char url[ADFS_MAX_PATH] = {0}; 
    char tmp_node[NAME_MAX] = {0};

    size_t vsize = 0;
    char *record = NULL;
    char *start = NULL;
    char *pos_sharp = NULL;
    char *pos_split = NULL;

    AIManager *pm = &g_manager;
    if (name_space)
        sprintf(key, "%s#%s", name_space, fname);
    else
        sprintf(key, "#%s", fname);

    record = kcdbget(pm->index_db, key, strlen(key), &vsize);
    if (record == NULL)
        return ADFS_ERROR;

    start = record;
    while (start)
    {
        pos_sharp = strstr(start, "#");
        pos_split = strstr(start, "|");

        memset(url, 0, ADFS_MAX_PATH);
        if (pos_split == NULL)
            strcpy(tmp_node, pos_sharp+1);
        else
            strncpy(tmp_node, pos_sharp + 1, (int)(pos_split - pos_sharp - 1));
        sprintf(url, "http://%s/delete/%s", tmp_node, fname);

        if (name_space)
        {
            strncpy(url, "?namespace=", ADFS_MAX_PATH);
            strncpy(url, name_space, ADFS_MAX_PATH);
        }

        if (aic_delete(mgr_getnode(tmp_node), url) == ADFS_ERROR)
        {
            // failed. roll back.
            ;
        }

        if (pos_split == NULL)
            break;
        else
            start = pos_split + 1;
    }
    kcdbremove(pm->index_db, key, strlen(key));
    strncat(key, ".delete", ADFS_MAX_PATH);
    kcdbset(pm->index_db, key, strlen(key), record, vsize);

    kcfree(record);
    return ADFS_OK;
}

////////////////////////////////////////////////////////////////////////////////
//private
static ANNameSpace * create_ns(const char *name_space) 
{
    DBG_PRINTSN("mgr-create 0");

    if (strlen(name_space) >= NAME_MAX)
        return NULL;

    ANManager *pm = &g_manager;

    char ns_path[ADFS_MAX_PATH] = {0};
    snprintf(ns_path, sizeof(ns_path), "%s/%s", pm->path, name_space);

    int node_num = count_kch(ns_path);

    // index db of ADFS-Node
    char indexdb_path[ADFS_MAX_PATH] = {0};
    if (snprintf(indexdb_path, sizeof(indexdb_path), "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                ns_path, pm->kc_apow, pm->kc_fbp, pm->kc_bnum *40, pm->kc_msiz) >= sizeof(indexdb_path))
    {
        return NULL;
    }
    
    DBG_PRINTSN("mgr-create 10");

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
    DBG_PRINTSN("mgr-create 20");
    DBG_PRINTS("node num: ");
    DBG_PRINTIN((long)node_num);

    char tmp_path[ADFS_MAX_PATH] = {0};
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
            DBG_PRINTS("create ro db file\n");
            if (pns->create(pns, i, tmp_path, strlen(tmp_path), S_READ_ONLY) == ADFS_ERROR)
                return NULL;
        }
        else
        {
            DBG_PRINTS("create rw db file\n");
            if (pns->create(pns, i, tmp_path, strlen(tmp_path), S_READ_WRITE) == ADFS_ERROR)
                return NULL;
        }
    }

    DBG_PRINTSN("mgr-create 30");
    
    pns->pre = pm->tail;
    pns->next = NULL;
    if (pm->tail)
        pm->tail->next = pns;
    else
        pm->head = pns;
    pm->tail = pns;

    return pns;
}


// private
static ADFS_RESULT split_db(ANNameSpace * pns)
{
    ANManager * pm = &g_manager;
    NodeDB * last_node = pns->tail;

    if (pns->switch_state(pns, last_node->id, S_READ_ONLY) == ADFS_ERROR)
        return ADFS_ERROR;

    char path[ADFS_MAX_PATH] = {0};
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
static ANNameSpace * get_ns(const char * name_space)
{
    DBG_PRINTSN("mgr-getns 1");
    ANNameSpace * tmp = g_manager.tail;
    while (tmp)
    {
#ifdef DEBUG
        static int a = 100;
#endif // DEBUG
        DBG_PRINTS("mgr-getns :");
        DBG_PRINTI((long)a++);
        DBG_PRINTSN(tmp->name);
        DBG_PRINTSN(name_space);

        if (strcmp(tmp->name, name_space) == 0)
            return tmp;
        tmp = tmp->pre;
    }

    DBG_PRINTSN("mgr-getns 2");
    return NULL;
}

