/* manager.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __MANAGER_H___
#define __MANAGER_H___

#include <pthread.h>
#include "namespace.h"
#include "../def.h"


typedef struct CNManager {
    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    char data_dir[32];
    char log_dir[32];
    char core_log[32];

    int another_running;

    pthread_rwlock_t ns_lock;

    struct CNNameSpace * head;
    struct CNNameSpace * tail;
}CNManager;

// manager.c
int GNm_init(const char * conf_file, unsigned long cache_size);
int GNm_exit();
int GNm_save(const char * name_space, const char *fname, size_t fname_len, void * fp, size_t fp_len);
int GNm_get(const char *name_space, const char *fname, void ** ppfile_data, size_t *pfile_size);
int GNm_length(const char *ns, const char *fname, unsigned long *pfile_len);
int GNm_erase(const char *name_space, const char *fname);

// update.c
int GNu_run();

#endif // __MANAGER_H___

