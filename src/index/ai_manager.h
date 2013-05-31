/* ai_manager.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "ai_zone.h"
#include "ai_stat.h"
#include "../include/adfs.h"


typedef struct AINameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    struct AINameSpace *pre;
    struct AINameSpace *next;
}AINameSpace;

typedef struct AIManager
{
    char path[ADFS_MAX_PATH];
    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    struct AIStat *stat_test;
    struct AIStat s_upload;
    struct AIStat s_download;
    struct AIStat s_delete;

    struct AINameSpace *ns_head;
    struct AINameSpace *ns_tail;
    struct AIZone *z_head;
    struct AIZone *z_tail;
}AIManager;


// manager.c
ADFS_RESULT aim_init(const char *file_conf, const char *path_db, unsigned long mem_size, unsigned long max_file_size);
void aim_exit();
ADFS_RESULT aim_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char * aim_download(const char *name_space, const char *fname, const char *history);
ADFS_RESULT aim_delete(const char *name_space, const char *fname);
char * aim_status();

// connect.c
ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
ADFS_RESULT aic_erase(AINode *pn, const char *url);
ADFS_RESULT aic_status(AINode *pn, const char *url);

#endif // __MANAGER_H__
