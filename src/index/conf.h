/*
 * conf.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __CONF_H__
#define __CONF_H__

#include <string.h>
#include "def.h"

ADFS_RESULT conf_read(const char * pfile, const char * s, char *buf, size_t len);
int conf_split(char *p, char *key, char *value);

#endif // __CONF_H__
