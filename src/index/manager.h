/* manager.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include <pthread.h>
#include "zone.h"
#include "meta.h"
#include "../def.h"

#define MNGR_DATA_DIR 		"data"
#define MNGR_LOG_DIR		"log"
#define MNGR_CORE_LOG		"icore.log"
#define MNGR_INSTANCE_F		"instance_id"
#define MNGR_TEAM_L_F		"team_id"
#define MNGR_TEAM_R_F		"team"

typedef struct CINameSpace {
    char name[ _DFS_NAMESPACE_LEN ];
    KCDB *index_db;

    struct CINameSpace *prev;
    struct CINameSpace *next;
}CINameSpace;

typedef struct CIManager {
    int another_running;

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;
    unsigned long max_file_size;

    /*
    char data_dir[32];
    char log_dir[32];
    char core_log[32];
    char instance[32];
    char team[32];
    */

    int primary;
    int exit_flag;
    pthread_t th_syn;
    char syn_dir[_DFS_MAX_LEN];

    char instance_id[64];
    char team_id[64];

    struct CINameSpace *ns_head;
    struct CINameSpace *ns_tail;
    struct CIZone *z_head;
    struct CIZone *z_tail;
}CIManager;


// manager.c
int 	GIm_init(const char *file_conf, const char *syn_dir, int role, long bnum, unsigned long mem_size, unsigned long max_file_size);
int 	GIm_exit();
int 	GIm_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
char *	GIm_download(const char *name_space, const char *fname, const char *history);
int	GIm_exist(const char *name_space, const char *fname);
int	GIm_delete(const char *name_space, const char *fname);
int 	GIm_setnode(const char *node_name, const char *attr_rw);
char * 	GIm_status();

// connect.c
int GIc_upload(CINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
int GIc_connect(CINode *pn, const char *url, FLAG_CONNECTION);

// update.c
int GIu_run();

// syn_primary.c
int GIsp_init();
int GIsp_release();
int GIsp_export(const char *, const char *, const char *, const char *);

// syn_secondary.c
int GIss_init();
int GIss_release();

#endif // __MANAGER_H__

