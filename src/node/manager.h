/* manager.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __MANAGER_H___
#define __MANAGER_H___

#include <pthread.h>
#include "../adfs.h"

typedef struct ANManager
{
    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;
    char data_dir[32];
    char log_dir[32];
    char core_log[32];
    int another_running;
    pthread_rwlock_t ns_lock;

    struct ANNameSpace * head;
    struct ANNameSpace * tail;
}ANManager;

// manager.c
int anm_init(const char * conf_file, unsigned long cache_size);
int anm_exit();
int anm_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len);
int anm_get(const char *name_space, const char *fname, void ** ppfile_data, size_t *pfile_size);
int anm_erase(const char *name_space, const char *fname);

#endif // __MANAGER_H___

