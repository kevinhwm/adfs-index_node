/* ai_manager.c
 *
 * huangtao@antiy.com
 */

#include <string.h>
#include <dirent.h>     // opendir
#include <sys/stat.h>   // mkdir
#include <kclangc.h>
#include <curl/curl.h>
#include "ai.h"

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

    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    return ADFS_OK;
}

ADFS_RESULT mgr_upload()
{
    return ADFS_OK;
}

void mgr_exit()
{
    curl_global_cleanup();
}

