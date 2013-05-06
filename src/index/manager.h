/* manager.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "ai.h"


typedef struct AINameSpace
{
    char name[ADFS_NAMESPACE_LEN];
    KCDB * index_db;
    struct AINameSpace *pre;
    struct AINameSpace *next;
}AINameSpace;

typedef struct AIStat 
{
    unsigned long stat_start;
    int stat_min;
    int pos_last; 	// last record 
    int *stat_count;

    void (*release)(struct AIStat *);
    int *(*get)(struct AIStat *, time_t *);
    void (*inc)(struct AIStat *);
}AIStat;

typedef struct AIManager
{
    char db_path[ADFS_MAX_PATH];	// database file path
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


ADFS_RESULT mgr_init(const char *file_conf, const char *path_db, unsigned long mem_size, unsigned long max_file_size);
void mgr_exit();

ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char * mgr_download(const char *name_space, const char *fname, const char *history);
ADFS_RESULT mgr_delete(const char *name_space, const char *fname);
char * mgr_status();

#endif // __MANAGER_H__
