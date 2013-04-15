/* Antiy Labs. Basic Platform R & D Center
 * ai.h
 *
 * huangtao@antiy.com
 */

#include <linux/limits.h>                   // PATH_MAX
#include "../include/adfs.h"
#include <kclangc.h>


typedef struct AINode
{
    char ip_port[64];

    struct AINode *pre;
    struct AINode *next;
}AINode;

typedef struct AIZone
{
    char name[NAME_MAX];
    int weight;
    int num;

    struct AINode *head;
    struct AINode *tail;

    struct AIZone *pre;
    struct AIZone *next;

    ADFS_RESULT (*create)(struct AIZone *, const char *);
    void (*release_all)(struct AIZone *);
    AINode * (*rand_choose)(struct AIZone *);
}AIZone;

typedef struct AIManager
{
    char path[PATH_MAX];
    KCDB * index_db;

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    struct AIZone *head;
    struct AIZone *tail;
}AIManager;


// ai_manager.c
ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size);
void mgr_exit();
ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, size_t fname_len, void *fp, size_t fp_len);

// ai_zone.c
ADFS_RESULT z_init(AIZone *_this, const char *name, int weight);

// ai_function.c
ADFS_RESULT get_conf(const char * pfile, const char * s, char *buf, size_t len);
int parse_conf(char *p, char *key, char *value);
void trim_left(char * p);
void trim_right(char * p);
ADFS_RESULT parse_filename(char * p);

// ai_connect.c
ADFS_RESULT aic_upload();

