/* ai.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __AI_H__
#define __AI_H__

#include "adfs.h"
#include "multipart_parser.h"   // parse the post data
#include "record.h"
#include "zone.h"               // about zone infomation
#include "manager.h"            // main function. only one object -> g_manager

// function.c
void trim_left_white(char * p);
void trim_right_white(char * p);
int get_filename_from_url(char *);

// connect.c
ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
ADFS_RESULT aic_status(AINode *pn, const char *url);

// conf.c
ADFS_RESULT conf_read(const char * pfile, const char * s, char *buf, size_t len);
int conf_split(char *p, char *key, char *value);

// log.c
typedef enum LOG_LEVEL{
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
}LOG_LEVEL;
int log_init(const char *filename);
void log_release();
void log_out(const char *module, const char *info, LOG_LEVEL level);

// stat.c
ADFS_RESULT ais_init(AIStat *ps, unsigned long, int);

#endif // __AI_H__

