/* Antiy Labs. Basic Platform R & D Center.
 * function.h
 *
 * huangtao@antiy.com
 */

#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#include "def.h"

void trim_left_white(char * p);
void trim_right_white(char * p);
//ADFS_RESULT parse_filename(char * p);
ADFS_RESULT get_filename_from_url(char *);

#endif // __FUNCTION_H__
