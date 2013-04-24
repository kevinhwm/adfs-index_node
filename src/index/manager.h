/* Antiy Labs. Basic Platform R & D Center.
 * manager.h
 *
 * huangtao@antiy.com
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "def.h"
#include "zone.h"


typedef struct AIManager
{
    char path[ADFS_MAX_PATH];
    KCDB * map_db;
    KCDB * index_db;

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    struct AIZone *head;
    struct AIZone *tail;
}AIManager;


ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size);
void mgr_exit();

ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char * mgr_download(const char *name_space, const char *fname);
ADFS_RESULT mgr_delete(const char *name_space, const char *fname);

#endif // __MANAGER_H__
