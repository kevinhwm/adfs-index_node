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


static ADFS_RESULT mgr_create(const char *conf_file);
static AINode * mgr_getnode(const char *);
static AIZone * mgr_choose_node(AIManager *pm, const char * record);

AIManager g_manager;


ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size)
{    
    // 验证conf_file是否存在
    // 配置文件格式，必要的配置项是否都存在
    // 取出所有的配置项，检查值是否在合法范围内
    // 
    
    AIManager *pm = &g_manager;

    memset(pm, 0, sizeof(*pm));
    if (strlen(path) > ADFS_MAX_PATH)
        return ADFS_ERROR;

    DIR *dirp = opendir(path);
    if( dirp == NULL ) 
        return ADFS_ERROR;
    closedir(dirp);

    strncpy(pm->path, path, sizeof(pm->path));
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;

    // create map db
    char mapdb_path[ADFS_MAX_PATH] = {0};
    sprintf(mapdb_path, "%s/mapdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->path, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    pm->map_db = kcdbnew();
    if (kcdbopen(pm->map_db, mapdb_path, KCOREADER|KCOWRITER|KCOCREATE) == 0)
        return ADFS_ERROR;

    // create index db
    char indexdb_path[ADFS_MAX_PATH] = {0};
    sprintf(indexdb_path, "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->path, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    pm->index_db = kcdbnew();
    if (kcdbopen(pm->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE) == 0)
        return ADFS_ERROR;

    if (mgr_create(conf_file) == ADFS_ERROR)
        return ADFS_ERROR;

    srand(time(NULL));
    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    return ADFS_OK;
}

void mgr_exit()
{
    AIManager *pm = &g_manager;

    AIZone *pz = pm->head;
    while (pz)
    {
        AIZone *tmp = pz;
        pz = pz->next;

        tmp->release_all(tmp);
        free(tmp);
    }
    kcdbclose(pm->map_db);
    kcdbdel(pm->map_db);
    kcdbclose(pm->index_db);
    kcdbdel(pm->index_db);

    curl_global_cleanup();
}

ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    int exist = 1;
    char map_key[ADFS_MAX_PATH] = {0};
    char index_key[ADFS_MAX_PATH] = {0};
    size_t vsize;
    char *map_record = NULL;
    char *index_record = NULL;
    char *new_record = NULL;
    AIManager *pm = &g_manager;

    if (name_space)
        sprintf(map_key, "%s#%s", name_space, fname);
    else
        sprintf(map_key, "#%s", fname);

    map_record = kcdbget(pm->map_db, map_key, strlen(map_key), &vsize);
    // uuid 存在
    // uuid| 不存在
    // uuid|uuid 存在
    if (map_record == NULL || map_record[vsize-1] == '|'])
        exist = 0;

    if (exist && !overwrite)             // exist and not overwrite
    {
        return ADFS_OK;
    }
    else if (exist && overwrite)         // exist and overwrite
    {
        // 取得当前的UUID
        // 删除当前uuid指定的文件
        // 修改map db中的记录，添加新的uuid
        // 上传新样本

        char url[ADFS_MAX_PATH] = {0};
        char tmp_node[NAME_MAX] = {0};

        char *start = record;
        char *pos_sharp = NULL;
        char *pos_split = NULL;
        while (start)
        {
            pos_sharp = strstr(start, "#");
            pos_split = strstr(start, "|");

            if (pos_split == NULL)
                strcpy(tmp_node, pos_sharp + 1);
            else
                strncpy(tmp_node, pos_sharp + 1, pos_split - pos_sharp -1);

            if (name_space)
                sprintf(url, "http://%s/%s?namespace=%s", tmp_node, fname, name_space);
            else
                sprintf(url, "http://%s/%s", tmp_node, fname);

            if (aic_upload(mgr_getnode(tmp_node), url, fname, fdata, fdata_len) == ADFS_ERROR)
            {
                printf("upload error: %s\n", url);
                // failed. roll back.
            }

            start = pos_split + 1;
        }
    }
    else                                        // not exist
    {
        char new_record[ADFS_MAX_PATH] = {0};
        AIZone *pz = pm->head;
        while (pz)
        {
            AINode * pn = pz->rand_choose(pz);
            char url[ADFS_MAX_PATH] = {0};
            if (name_space)
                sprintf(url, "http://%s/upload_file/%s?namespace=%s", pn->ip_port, fname, name_space);
            else
                sprintf(url, "http://%s/upload_file/%s", pn->ip_port, fname);
            if (aic_upload(pn, url, fname, fdata, fdata_len) == ADFS_ERROR)
            {
                printf("upload error: %s\n", url);
                // failed. roll back.
            }
            strncat(new_record, pz->name, sizeof(new_record));
            strcat(new_record, "#");
            strncat(new_record, pn->ip_port, sizeof(new_record));
            strcat(new_record, "|");
            pz = pz->next;
        }
        new_record[strlen(new_record)-1] = '\0';

        int32_t res = kcdbset(pm->index_db, key, strlen(key), new_record, strlen(new_record));
        if (res == 0)
        {
            // failed. roll back.
        }
    }

    kcfree(record);
    return ADFS_OK;
}

// return value:
// NULL:    file not found
// url :    "char *" must be freed by caller
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

    AIZone *pz = mgr_choose_node(pm, record);
    if (pz == NULL)
    {
        kcfree(record);
        return NULL;
    }

    char *url = (char *)malloc(ADFS_MAX_PATH);
    if (url == NULL)
    {
        kcfree(record);
        return NULL;
    }

    char *start = strstr(record, pz->name);      // record like: zone#node|zone#node
    char *pos_sharp = strstr(start, "#");
    char *pos_split = strstr(start, "|");

    memset(url, 0, ADFS_MAX_PATH);
    if (pos_split == NULL)
        sprintf(url, "http://%s/download/%s", pos_sharp+1, fname);
    else
        sprintf(url, "http://%.*s/download/%s", (int)(pos_split-pos_sharp-1), pos_sharp+1, fname);

    if (name_space)
    {
        strncpy(url, "?namespace=", ADFS_MAX_PATH);
        strncpy(url, name_space, ADFS_MAX_PATH);
    }

    kcfree(record);
    return url;
}

ADFS_RESULT mgr_delete(const char *name_space, const char *fname)
{
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

// private
static ADFS_RESULT mgr_create(const char *conf_file)
{
    AIManager *pm = &g_manager;

    char value[NAME_MAX] = {0};

    if (get_conf(conf_file, "zone_num", value, sizeof(value)) == ADFS_ERROR)
        return ADFS_ERROR;

    int zone_num = atoi(value);
    if (zone_num <= 0)
        return ADFS_ERROR;

    for (int i=0; i<zone_num; ++i)
    {
        char key[NAME_MAX] = {0};
        char name[NAME_MAX] = {0};
        char weight[NAME_MAX] = {0};
        char num[NAME_MAX] = {0};

        snprintf(key, sizeof(key), "zone%d_name", i);
        if (get_conf(conf_file, key, name, sizeof(name)) == ADFS_ERROR)
            return ADFS_ERROR;

        snprintf(key, sizeof(key), "zone%d_weight", i);
        if (get_conf(conf_file, key, weight, sizeof(weight)) == ADFS_ERROR)
            return ADFS_ERROR;

        AIZone *pz = (AIZone *)malloc(sizeof(AIZone));
        if (z_init(pz, name, atoi(weight)) == ADFS_ERROR)
            return ADFS_ERROR;

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

        int node_num = atoi(num);
        if (node_num <= 0)
            return ADFS_ERROR;

        for (int j=0; j<node_num; ++j)
        {
            snprintf(key, sizeof(key), "zone%d_%d", i, j);
            if (get_conf(conf_file, key, value, sizeof(value)) == ADFS_ERROR)
                return ADFS_ERROR;
            if (strlen(value) <= 0)
                return ADFS_ERROR;
            if (pz->create(pz, value) == ADFS_ERROR)
                return ADFS_ERROR;
        }
    }
    return ADFS_OK;
}

// private
static AIZone * mgr_choose_node(AIManager *pm, const char *record)
{
    AIZone *biggest_z = NULL;   // (weight/count)
    AIZone *least_z = NULL;

    AIZone *pz = pm->head;
    while (pz)
    {
        if (strstr(record, pz->name) == NULL)
            continue;

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

            if (least_z == NULL)
                least_z = pz;
            else
                least_z = (pz->weight / pz->count) < (least_z->weight / least_z->count) ? pz : least_z;
        }
        pz = pz->next;
    }
    biggest_z->count += 1;

    // 对于互质的 weight1 和 weight2，
    // 不存在 count1 和 count2, (count属于正整数范围内且 weight > count > 0),
    // 使(weight1/count1) = (weight2/count2)
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

// private
static AINode * mgr_getnode(const char *node)
{
    AIZone *pz = g_manager.head;
    while (pz)
    {
        AINode * pn = pz->head;
        while (pn)
        {
            if (strcmp(pn->ip_port, node) == 0)
                return pn;
            pn = pn->next;
        }
        pz = pz->next;
    }
    return NULL;
}
