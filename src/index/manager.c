/* 
 * manager.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <string.h>
#include <dirent.h>     // opendir
#include <sys/stat.h>   // mkdir
#include <kclangc.h>
#include <curl/curl.h>
#include <time.h>
#include "ai.h"


static ADFS_RESULT mgr_init_zone(const char *file_conf);
static AIZone * mgr_create_zone(const char *name, int weight);
static AINameSpace * mgr_create_ns(const char *name);
static AINode * mgr_get_node(const char *node);
static AINameSpace * mgr_get_ns(const char *ns);
static AIZone * mgr_choose_zone(const char * record);

AIManager g_manager;


ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size)
{    
    AIManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));

    if (strlen(path) > ADFS_MAX_PATH) {
	pm->msg = MSG_FAIL_LONG_PATH;
        return ADFS_ERROR;
    }

    DIR *dirp = opendir(path);
    if( dirp == NULL ) {
	pm->msg = MSG_FAIL_NO_PATH;
        return ADFS_ERROR;
    }
    closedir(dirp);

    strncpy(pm->db_path, path, sizeof(pm->db_path));
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;

    if (mgr_init_zone(conf_file) == ADFS_ERROR)
        return ADFS_ERROR;

    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    srand(time(NULL));
    return ADFS_OK;
}

void mgr_exit()
{
    AIManager *pm = &g_manager;

    AIZone *pz = pm->z_head;
    while (pz)
    {
        AIZone *tmp = pz;
        pz = pz->next;

        tmp->release_all(tmp);
        free(tmp);
    }

    AINameSpace *pns = pm->ns_head;
    while (pns)
    {
        AINameSpace *tmp = pns;
        pns = pns->next;

	kcdbclose(tmp->index_db);
	kcdbdel(tmp->index_db);
        free(tmp);
    }

    curl_global_cleanup();
}

ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    AIManager *pm = &g_manager;

    int exist = 1;
    char index_key[NAME_MAX] = {0};
    size_t old_list_len;
    char *old_list = NULL;
    char *new_list = NULL;

    AINameSpace *pns = NULL;
    if (name_space)
	pns = mgr_get_ns(name_space);
    else
	pns = mgr_get_ns("default");

    if (pns == NULL) {
	pm->msg = MSG_FAIL_NAMESPACE;
	return ADFS_ERROR;
    }

    old_list = kcdbget(pns->index_db, fname, strlen(fname), &old_list_len);
    if (old_list == NULL || old_list[vsize-1] == '|')
        exist = 0;

    if (exist && !overwrite)		// exist and not overwrite
        return ADFS_OK;

    // send file

    // add record
    if (old_list == NULL)
    {
	kcdbset(pns->index_db, fname, strlen(fname), );
    }
    else
    {
	malloc();
	strncat();
	kcdbset(pns->index_db, fname, strlen(fname), );
    }

    /*
    if (exist && overwrite)	// exist and overwrite
    {
	char record[ADFS_MAX_PATH] = {0};
	create_time_string(record, sizeof(record));// length of this uuid is exactly 24 bytes
	strcpy(record, "#");
	strcpy(record, pns->name);	// length of this uuid is exactly 24 bytes
	strcpy(record, "#");

        char url[ADFS_MAX_PATH] = {0};
        char tmp_node[NAME_MAX] = {0};

        char *start = old_list;
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

            if (aic_upload(mgr_get_node(tmp_node), url, fname, fdata, fdata_len) == ADFS_ERROR)
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
        AIZone *pz = pm->z_head;
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
    */

    kcfree(old_list);
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

        if (aic_delete(mgr_get_node(tmp_node), url) == ADFS_ERROR)
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
static ADFS_RESULT mgr_init_zone(const char *conf_file)
{
    AIManager *pm = &g_manager;
    pm->msg = MSG_FAIL_CONFIG;

    char value[NAME_MAX] = {0};

    // create zone
    if (conf_read(conf_file, "zone_num", value, sizeof(value)) == ADFS_ERROR)
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
        if (conf_read(conf_file, key, name, sizeof(name)) == ADFS_ERROR)
            return ADFS_ERROR;

        snprintf(key, sizeof(key), "zone%d_weight", i);
        if (conf_read(conf_file, key, weight, sizeof(weight)) == ADFS_ERROR)
            return ADFS_ERROR;

	AIZone *pz = mgr_create_zone(name, atoi(weight));
	if (pz == NULL)
	    return ADFS_ERROR;

        snprintf(key, sizeof(key), "zone%d_num", i);
        if (conf_read(conf_file, key, num, sizeof(num)) == ADFS_ERROR)
            return ADFS_ERROR;

        int node_num = atoi(num);
        if (node_num <= 0)
            return ADFS_ERROR;

        for (int j=0; j<node_num; ++j)
        {
            snprintf(key, sizeof(key), "zone%d_%d", i, j);
            if (conf_read(conf_file, key, value, sizeof(value)) == ADFS_ERROR)
                return ADFS_ERROR;
            if (strlen(value) <= 0)
                return ADFS_ERROR;
            if (pz->create(pz, value) == ADFS_ERROR) 
                return ADFS_ERROR;
        }
    }

    if (conf_read(conf_file, "namespace_num", value, sizeof(value)) == ADFS_ERROR)
        return ADFS_ERROR;
    int ns_num = atoi(value);
    if (ns_num <= 0 || ns_num >100)
        return ADFS_ERROR;

    // create namespace
    if (mgr_create_ns("default") == ADFS_ERROR)
	return ADFS_ERROR;
    for (int i=0; i<ns_num; ++i)
    {
        char key[NAME_MAX] = {0};
	sprintf(key, "namespace_%d", i);
	if (conf_read(conf_file, key, value, sizeof(value)) == ADFS_ERROR)
	    return ADFS_ERROR;

	if (mgr_create_ns(value) == ADFS_ERROR)
	    return ADFS_ERROR;
    }
	
    return ADFS_OK;
}

// private
static AIZone * mgr_choose_zone(const char *record)
{
    AIManager *pm = &g_manager;
    AIZone *biggest_z = NULL;   // (weight/count)
    AIZone *least_z = NULL;

    AIZone *pz = pm->z_head;
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
        pz = pm->z_head;
        while (pz)
        {
            pz->count = 0;
            pz = pz->next;
        }
    }

    return biggest_z;
}

// private
static AINode * mgr_get_node(const char *node)
{
    AIZone *pz = g_manager.z_head;
    while (pz)
    {
        AINode * pn = pz->z_head;
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

// private
static AINameSpace * mgr_get_ns(const char *ns)
{
    AINameSpace *pns = g_manager.ns_head;
    while (pns)
    {
	if (strcmp(pns->name, ns) == 0)
	    return pns;
	pns = pns->next;
    }
    return NULL;
}

// private
static AIZone * mgr_create_zone(const char *name, int weight)
{
    AIManager * pm = &g_manager;

    AIZone *pz = (AIZone *)malloc(sizeof(AIZone));
    if (pz == NULL) {
	pm->msg == MSG_ERR_MALLOC;
	return NULL;
    }

    z_init(pz, name, weight);

    pz->pre = pm->z_tail;
    pz->next = NULL;
    if (pm->z_tail)
	pm->z_tail->next = pz;
    else
	pm->z_head = pz;
    pm->z_tail = pz;

    return pz;
}

// private
static ADFS_RESULT mgr_create_ns(const char *name)
{
    AIManager * pm = &g_manager;

    AINameSpace *pns = pm->ns_head;
    while (pns)
    {
    	if (strcmp(pns->name, name) == 0)
	    return ADFS_ERROR;
	pns = pns->next;
    }

    pns = (AIZone *)malloc(sizeof(AINameSpace));
    if (pns == NULL) {
	pm->msg == MSG_ERR_MALLOC;
	return ADFS_ERROR;
    }

    strncpy(pns->name, name, sizeof(pns->name));

    char indexdb_path[ADFS_MAX_PATH] = {0};
    sprintf(indexdb_path, "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->db_path, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    pm->index_db = kcdbnew();
    if (kcdbopen(pm->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE) == 0) {
	pm->msg = MSG_FAIL_OPEN_DB;
        return ADFS_ERROR;
    }

    pns->pre = pm->ns_tail;
    pns->next = NULL;
    if (pm->ns_tail)
	pm->ns_tail->next = pns;
    else
	pm->ns_head = pns;
    pm->ns_tail = pns;

    return ADFS_OK;
}

