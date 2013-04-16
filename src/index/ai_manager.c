/* Antiy Labs. Basic Platform R & D Center
 * ai_manager.c
 *
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>     // opendir
#include <sys/stat.h>   // mkdir
#include <kclangc.h>
#include <curl/curl.h>
#include <time.h>
#include "ai.h"


static ADFS_RESULT mgr_create(const char *conf_file);
static ADFS_RESULT mgr_decide(AIManager *pm, DS_List ** ppdsl);
static AIZone * mgr_choose_node(AIManager *pm);

AIManager g_manager;


ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size)
{    
    printf("mgr-init 0\n");
    AIManager *pm = &g_manager;

    memset(pm, 0, sizeof(*pm));
    if (strlen(path) > ADFS_MAX_PATH)
        return ADFS_ERROR;

    printf("mgr-init 10\n");
    DIR *dirp = opendir(path);
    if( dirp == NULL ) 
        return ADFS_ERROR;
    closedir(dirp);

    printf("mgr-init 20\n");
    strncpy(pm->path, path, sizeof(pm->path));
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;
    char indexdb_path[ADFS_MAX_PATH] = {0};
    sprintf(indexdb_path, "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->path, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);

    pm->index_db = kcdbnew();
    if (kcdbopen(pm->index_db, indexdb_path, KCOWRITER|KCOCREATE) == 0)
        return ADFS_ERROR;
    printf("mgr-init 30\n");

    if (mgr_create(conf_file) == ADFS_ERROR)
        return ADFS_ERROR;

    printf("mgr-init 40\n");
    srand(time(NULL));
    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    return ADFS_OK;
}

ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    AIManager *pm = &g_manager;
    
    char key[ADFS_MAX_PATH] = {0};
    if (name_space)
        sprintf(key, "%s#%s", name_space, fname);
    else
        sprintf(key, "#%s", fname);

    size_t vsize;
    char *record = kcdbget(pm->index_db, key, strlen(key), &vsize);
    printf("search db: %s\nkey: %s\n", record, key);
    if (record != NULL && !overwrite)             // exist and not overwrite
    {
        return ADFS_OK;
    }
    else if (record != NULL && overwrite)         // exist and overwrite
    {
        char *start = record;
        char *pos_split = NULL;
        char *pos_sharp = NULL;
        DS_List * node_list = NULL;
        while ((pos_sharp = strstr(start, "#")))
        {
            pos_split = strstr(pos_sharp, "|");
            list_add(&node_list, start, pos_sharp - start, pos_sharp + 1, pos_split - pos_sharp -1);
            start = pos_split + 1;
        }

        char url[ADFS_MAX_PATH] = {0};
        DS_List *tmp = node_list;
        while (tmp)
        {
            if (name_space)
                sprintf(url, "http://%s/%s?namespace=%s", tmp->node, fname, name_space);
            else
                sprintf(url, "http://%s/%s", tmp->node, fname);

            if (aic_upload(url, fname, fdata, fdata_len) == ADFS_ERROR)
            {
                // failed
                // roll back
            }
            tmp = tmp->next;
        }

        list_free(node_list);
    }
    else                                        // not exist
    {
        DS_List *node_list = NULL;
        if (mgr_decide(pm, &node_list) == ADFS_ERROR)
        {
            // failed
            // roll back
        }

        char record[ADFS_MAX_PATH] = {0};
        char url[ADFS_MAX_PATH] = {0};
        DS_List *tmp = node_list;
        while (tmp)
        {
            if (name_space)
                sprintf(url, "http://%s/upload_file/%s?namespace=%s", tmp->node, fname, name_space);
            else
                sprintf(url, "http://%s/upload_file/%s", tmp->node, fname);

            printf("%s\n", url);
            if (aic_upload(url, fname, fdata, fdata_len) == ADFS_ERROR)
            {
                // failed
                // roll back
            }

            strncat(record, tmp->zone, sizeof(record));
            strcat(record, "#");
            strncat(record, tmp->node, sizeof(record));
            strcat(record, "|");

            tmp = tmp->next;
        }
        record[strlen(record)-1] = '\0';
        printf("record: %s\n", record);
        int32_t res = kcdbset(pm->index_db, key, strlen(key), record, strlen(record));
        if (res == 0)
        {
            // failed
            // roll back
        }

        list_free(node_list);
    }

    kcfree(record);

    return ADFS_OK;
}

char * mgr_download(const char *name_space, const char *fname)
{
    AIManager *pm = &g_manager;

    char key[ADFS_MAX_PATH] = {0};
    if (name_space)
        sprintf(key, "%s#%s", name_space, fname);
    else
        sprintf(key, "#%s", fname);

    size_t vsize;
    char *record = kcdbget(pm->index_db, key, strlen(key), &vsize);
    if (record == NULL)
        return NULL;

    AIZone *pz = mgr_choose_node(pm);
    if (pz == NULL)
        return NULL;

    char *pos1 = strstr(record, pz->name);         // record like: zone#node|zone#node
    char *pos2 = strstr(pos1, "#");
    char *pos3 = strstr(pos1, "|");

    char url[ADFS_MAX_PATH] = {0};
    if (name_space)
        sprintf(url, "http://%.*s/%s?namespace=%s", (int)(pos3-pos2-1), pos2+1, fname, name_space);
    else
        sprintf(url, "http://%.*s/%s", (int)(pos3-pos2), pos1+1, fname);

    kcfree(record);
    return url;
}

void mgr_exit()
{
    AIManager *pm = &g_manager;

    printf("exit 10\n");
    AIZone *pz = pm->head;
    while (pz)
    {
        AIZone *tmp = pz;
        pz = pz->next;

        tmp->release_all(tmp);
        free(tmp);
    }
    printf("exit 20\n");
    kcdbclose(pm->index_db);
    printf("exit 30\n");
    kcdbdel(pm->index_db);

    printf("exit 40\n");
    curl_global_cleanup();
}

// private
static ADFS_RESULT mgr_create(const char *conf_file)
{
    printf("mgr-create 10\n");
    AIManager *pm = &g_manager;

    char value[NAME_MAX] = {0};

    if (get_conf(conf_file, "zone_num", value, sizeof(value)) == ADFS_ERROR)
        return ADFS_ERROR;

    printf("mgr-create 20\n");
    int zone_num = atoi(value);
    if (zone_num <= 0)
        return ADFS_ERROR;
    
    printf("mgr-create 30\n");
    for (int i=0; i<zone_num; ++i)
    {
        char key[NAME_MAX] = {0};
        char name[NAME_MAX] = {0};
        char weight[NAME_MAX] = {0};
        char num[NAME_MAX] = {0};

        printf("mgr-create 40\n");
        snprintf(key, sizeof(key), "zone%d_name", i);

        printf("%s\n", conf_file);
        printf("%s\n", key);
        printf("%s\n", name);

        if (get_conf(conf_file, key, name, sizeof(name)) == ADFS_ERROR)
            return ADFS_ERROR;

        printf("mgr-create 50\n");
        snprintf(key, sizeof(key), "zone%d_weight", i);
        if (get_conf(conf_file, key, weight, sizeof(weight)) == ADFS_ERROR)
            return ADFS_ERROR;

        printf("mgr-create 60\n");
        AIZone *pz = (AIZone *)malloc(sizeof(AIZone));
        if (z_init(pz, name, atoi(weight)) == ADFS_ERROR)
            return ADFS_ERROR;

        printf("mgr-create 70\n");
        pz->pre = pm->tail;
        pz->next = NULL;
        if (pm->tail)
            pm->tail->next = pz;
        else
            pm->head = pz;
        pm->tail = pz;

        snprintf(key, sizeof(key), "zone%d_num", i);
        if (get_conf(conf_file, key, num, sizeof(num)) == ADFS_ERROR)
            return ADFS_ERROR;

        printf("mgr-create 80\n");
        int node_num = atoi(num);
        if (node_num <= 0)
            return ADFS_ERROR;

        printf("mgr-create 90\n");
        for (int j=0; j<node_num; ++j)
        {
            printf("i=%d, j=%d\n", i, j);
            snprintf(key, sizeof(key), "zone%d_%d", i, j);
            if (get_conf(conf_file, key, value, sizeof(value)) == ADFS_ERROR)
                return ADFS_ERROR;
            printf("mgr-create 100\n");
            if (strlen(value) <= 0)
                return ADFS_ERROR;
            printf("mgr-create 110\n");
            if (pz->create(pz, value) == ADFS_ERROR)
                return ADFS_ERROR;
        }
        printf("mgr-create 120\n");
    }
    return ADFS_OK;
}

static ADFS_RESULT mgr_decide(AIManager *pm, DS_List ** ppdsl)
{
    ADFS_RESULT res = ADFS_ERROR;
    AIZone *pz = pm->head;
    while (pz)
    {
        AINode * pn = pz->rand_choose(pz);
        if (pn == NULL)
        {
            // failed. roll back.
            ;
        }

        list_add(ppdsl, pz->name, strlen(pz->name), pn->ip_port, strlen(pn->ip_port));
        pz = pz->next;
    }
    return res;
}

static AIZone * mgr_choose_node(AIManager *pm)
{
    AIZone *pz = pm->head;

    AIZone *biggest_z = NULL;
    AIZone *least_z = NULL;
    while (pz)
    {
        if (pz->count == 0)
        {
            pz->count += 1;
            return pz;
        }
        else if (biggest_z == NULL)
            biggest_z = pz;
        else
        {
            biggest_z = (pz->weight / pz->count) > (biggest_z->weight / biggest_z->count) ? pz : biggest_z;
            least_z = (pz->weight / pz->count) < (least_z->weight / least_z->count) ? pz : least_z;
        }
        pz = pz->next;
    }
    biggest_z->count += 1;

    if (biggest_z == least_z)
    {
        pz = pm->head;
        while (pz)
        {
            pz->count = 0;
            pz = pz->next;
        }
    }

    return biggest_z;
}

