/* Antiy Labs. Basic Platform R & D Center.
 * connect.h
 *
 * huangtao@antiy.com
 */

#ifndef __CONNECT_H__
#define __CONNECT_H__

#include "def.h"
#include "zone.h"

ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len);
ADFS_RESULT aic_delete(AINode *pn, const char *url);

#endif // __CONNECT_H__
