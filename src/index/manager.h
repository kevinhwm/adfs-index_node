/* manager.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "zone.h"
#include "../adfs.h"


typedef struct AINameSpace {
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    struct AINameSpace *prev;
    struct AINameSpace *next;
}AINameSpace;

typedef struct AIManager {
    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;
    unsigned long max_file_size;

    char data_dir[32];
    char log_dir[32];
    char core_log[32];

    int another_running;

    struct AINameSpace *ns_head;
    struct AINameSpace *ns_tail;
    struct AIZone *z_head;
    struct AIZone *z_tail;
}AIManager;


// manager.c
int 	aim_init(const char *file_conf, long bnum, unsigned long mem_size, unsigned long max_file_size);
int 	aim_exit();
int 	aim_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char *	aim_download(const char *name_space, const char *fname, const char *history);
int	aim_delete(const char *name_space, const char *fname);
int	aim_exist(const char *name_space, const char *fname);
char * 	aim_status();

// connect.c
int aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
int aic_connect(AINode *pn, const char *url, FLAG_CONNECTION);

// update.c
int aiu_init();

#endif // __MANAGER_H__

