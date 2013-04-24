/* Antiy Labs. Basic Platform R & D Center.
 * function.h
 *
 * huangtao@antiy.com
 */

#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "def.h"

ADFS_RESULT get_conf(const char * pfile, const char * s, char *buf, size_t len);
int parse_conf(char *p, char *key, char *value);
void trim_left(char * p);
void trim_right(char * p);
ADFS_RESULT parse_filename(char * p);

#endif // __FUNCTION_H__
