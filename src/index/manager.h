/* manager.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "zone.h"
#include "stat.h"
#include "../adfs.h"


typedef struct AINameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    struct AINameSpace *prev;
    struct AINameSpace *next;
}AINameSpace;

typedef struct AIManager
{
    char data_dir[ADFS_MAX_LEN];
    char log_dir[ADFS_MAX_LEN];
    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    struct AIStat s_upload;
    struct AIStat s_download;
    struct AIStat s_delete;

    struct AINameSpace *ns_head;
    struct AINameSpace *ns_tail;
    struct AIZone *z_head;
    struct AIZone *z_tail;
}AIManager;


// manager.c
ADFS_RESULT aim_init(const char *file_conf, const char *data_dir, long bnum, unsigned long mem_size, unsigned long max_file_size);
void aim_exit();
ADFS_RESULT aim_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char * aim_download(const char *name_space, const char *fname, const char *history);
ADFS_RESULT aim_delete(const char *name_space, const char *fname);
int aim_exist(const char *name_space, const char *fname);
char * aim_status();

// connect.c
ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
ADFS_RESULT aic_connect(AINode *pn, const char *url, FLAG_CONNECTION);

// update.c
int aiu_init();

#endif // __MANAGER_H__
