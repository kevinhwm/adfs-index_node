/* an_manager.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __MANAGER_H___
#define __MANAGER_H___

#include <pthread.h>
#include "../include/adfs.h"

typedef struct ANManager
{
    char path[ADFS_MAX_PATH];
    pthread_rwlock_t ns_lock;
    struct ANNameSpace * head;
    struct ANNameSpace * tail;

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;
}ANManager;

// an_manager.c
ADFS_RESULT anm_init(const char * conf_file, const char * dbpath, unsigned long cache_size);
void anm_exit();
ADFS_RESULT anm_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len);
void anm_get(const char *name_space, const char *fname, void ** ppfile_data, size_t *pfile_size);
ADFS_RESULT anm_erase(const char *name_space, const char *fname);
ADFS_RESULT anm_syn();

#endif // __MANAGER_H___

