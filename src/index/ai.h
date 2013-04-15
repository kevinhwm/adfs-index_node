/* Antiy Labs. Basic Platform R & D Center
 * ai.h
 *
 * huangtao@antiy.com
 */

#include "../include/adfs.h"
#include <linux/limits.h>                   // PATH_MAX


typedef struct AINode
{
    char ip_port[64];

    struct AINode *pre;
    struct AINode *next;
}AINode;

typedef struct AIZone
{
    char name[NAME_MAX];
    int num;
    int weight;

    struct AINode *first;
    struct AINode *last;

    struct AIZone *pre;
    struct AIZone *next;
}AIZone;

typedef struct AIManager
{
    char path[PATH_MAX];

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

}AIManager;


// ai_manager.c
ADFS_RESULT mgr_init(const char *conf_file, const char *path, unsigned long mem_size);
void mgr_exit();
ADFS_RESULT mgr_upload(const char *name_space, const char *fname, size_t fname_len, void *fp, size_t fp_len);
void mgr_exit();

// ai_function.c
int get_conf(const char * pfile, const char * s, char *buf, size_t len);
void trim_left(char * p);
void trim_right(char * p);
int parse_conf(char *p, char *key, char *value);
ADFS_RESULT parse_filename(char * p);

// ai_connect.c
ADFS_RESULT aic_upload();

