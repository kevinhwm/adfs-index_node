/* ai.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __AI_H__
#define __AI_H__


#include "adfs.h"
#include "manager.h"            // main function. only one object -> g_manager
#include "multipart_parser.h"   // parse the post data
#include "record.h"
#include "zone.h"               // about zone infomation


// function.c
void trim_left_white(char * p);
void trim_right_white(char * p);
int get_filename_from_url(char *);

// connect.c
ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
ADFS_RESULT aic_delete(AINode *pn, const char *url);

// conf.h
ADFS_RESULT conf_read(const char * pfile, const char * s, char *buf, size_t len);
int conf_split(char *p, char *key, char *value);

#endif // __AI_H__

