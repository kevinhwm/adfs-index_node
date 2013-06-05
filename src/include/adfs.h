/* adfs.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#define ADFS_VERSION		"3.1.4"
#define ADFS_MAX_PATH		1024
#define ADFS_FILENAME_LEN	256
#define ADFS_ZONENAME_LEN	128
#define ADFS_NAMESPACE_LEN	128
#define ADFS_NODENAME_LEN	128
#define ADFS_UUID_LEN		24		// exactly 24 bytes

#include <string.h>	// size_t
#include "cJSON.h"

typedef enum {
    ADFS_ERROR	= -1,
    ADFS_OK	= 0,
}ADFS_RESULT;

//==============================================================================

// ai_function.c
cJSON * conf_parse(const char *conf_file);
void conf_release(cJSON *json);
int get_filename_from_url(char * p);


// log.c
typedef enum {
    LOG_LEVEL_SYSTEM = 0,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
}LOG_LEVEL;

int log_init(const char *filename);
void log_release();
void log_out(const char *module, const char *info, LOG_LEVEL level);

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

