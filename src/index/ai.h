/* ai.h
 *
 * huangtao@antiy.com
 */

#include "../include/adfs.h"
#include <linux/limits.h>                   // PATH_MAX

#define SPLIT_FILE_SIZE     0x04000000      // 2^2 * 16^6 = 2^26 = 64MB


typedef struct AINode
{
    int d;
}AINode;

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

// ai_function.c
int get_conf(const char * pfile, const char * s, char *buf, size_t len);
void trim_left(char * p);
void trim_right(char * p);
int parse_conf(char *p, char *key, char *value);
ADFS_RESULT parse_filename(char * p);

// ai_connect.c
ADFS_RESULT ai_save();
ADFS_RESULT ai_get();

