/* adfs.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#define ADFS_VERSION		"3.3.0"

#define ADFS_MAX_LEN		1024
#define ADFS_FILENAME_LEN	512
#define ADFS_ZONENAME_LEN	128
#define ADFS_NAMESPACE_LEN	128
#define ADFS_NODENAME_LEN	128
#define ADFS_UUID_LEN		24		// exactly 24 bytes

#define ADFS_RUNNING_FLAG	"adfs.flag"

#include "cJSON.h"
#include "log.h"

//==============================================================================

// function.c
cJSON * conf_parse(const char *conf_file);
void conf_release(cJSON *json);
int get_filename_from_url(char * p, char *pattern);

//==============================================================================

#ifdef DEBUG
#include <stdio.h>

#define DBG_PRINTS(x)	printf("%s", x)
#define DBG_PRINTSN(x)	printf("%s\n", x)
#define DBG_PRINTI(x)	printf("%d", x)
#define DBG_PRINTIN(x)	printf("%d\n", x)
#define DBG_PRINTU(x)	printf("%lu", x)
#define DBG_PRINTUN(x)	printf("%lu\n", x)
#define DBG_PRINTP(x)	printf("%p", x)
#define DBG_PRINTPN(x)	printf("%p\n", x)
#else
#define DBG_PRINTS(x)
#define DBG_PRINTSN(x)
#define DBG_PRINTI(x)
#define DBG_PRINTIN(x)
#define DBG_PRINTU(x)
#define DBG_PRINTUN(x)
#define DBG_PRINTP(x)
#define DBG_PRINTPN(x)
#endif // DEBUG

#endif // __ADFS_H__

