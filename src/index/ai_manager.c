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
#include "ai.h"


static ADFS_RESULT mgr_create(const char *conf_file);

AIManager g_manager;


ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size)
{    
    memset(&g_manager, 0, sizeof(g_manager));
    if (strlen(path) > PATH_MAX)
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

    if (mgr_create(conf_file) == ADFS_ERROR)
        return ADFS_ERROR;

    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    return ADFS_OK;
}

ADFS_RESULT mgr_upload(const char *name_space, const char *fname, size_t fname_len, void *fp, size_t fp_len)
{
    char url[PATH_MAX] = {0};
    
    // traverse all zone
    //
    //   choose a node to save
    //

    strncpy(url, fname, sizeof(url));
    if (name_space)
    {
        strncpy(url, "?namespace=", sizeof(url));
        strncpy(url, name_space, sizeof(url));
    }

    return ADFS_OK;
}
void mgr_exit()
{
    AIManager *pm = &g_manager;

    AIZone *pz = pm->tail;
    while (pz)
    {
        AIZone *tmp = pz->pre;
        tmp->release_all(tmp);
        free(pz);
        pz = tmp;
    }

    curl_global_cleanup();
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

        for (int j=0; j<node_num; ++i)
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

