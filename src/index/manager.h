/* manager.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "adfs.h"
#include "zone.h"


typedef struct AINameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    struct AINameSpace *pre;
    struct AINameSpace *next;
}AINameSpace;

typedef struct AIManager
{
    const char *msg;
    char db_path[ADFS_MAX_PATH];	// database file path
    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    struct AINameSpace *ns_head;
    struct AINameSpace *ns_tail;
    struct AIZone *z_head;
    struct AIZone *z_tail;
}AIManager;


ADFS_RESULT mgr_init(const char *file_conf, const char *path_db, unsigned long mem_size);
// mgr_check();
// clean up function
void mgr_exit();

ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char * mgr_download(const char *name_space, const char *fname);
ADFS_RESULT mgr_delete(const char *name_space, const char *fname);

#endif // __MANAGER_H__
