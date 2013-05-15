/* adfs.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#define ADFS_VERSION		"3.0"
#define ADFS_MAX_PATH		1024
#define ADFS_FILENAME_LEN	256
#define ADFS_NODENAME_LEN	24
#define ADFS_ZONENAME_LEN	128
#define ADFS_NAMESPACE_LEN	128
#define ADFS_UUID_LEN		24		// exactly 24 bytes

#include <string.h>	// size_t

typedef enum {
    ADFS_ERROR	= -1,
    ADFS_OK	= 0,
}ADFS_RESULT;

//==============================================================================

// conf.c
ADFS_RESULT conf_read(const char * pfile, const char * s, char *buf, size_t len);
int conf_split(char *p, char *key, char *value);

// function.c
void trim_left_white(char * p);
void trim_right_white(char * p);
int get_filename_from_url(char *);

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
#define DBG_PRINTI(x)	printf("%ld", x)
#define DBG_PRINTIN(x)	printf("%ld\n", x)
#define DBG_PRINTZ(x)	printf("%z", x)
#define DBG_PRINTZN(x)	printf("%z\n", x)
#define DBG_PRINTP(x)	printf("%p", x)
#define DBG_PRINTPN(x)	printf("%p\n", x)
#else
#define DBG_PRINTS(x)
#define DBG_PRINTSN(x)
#define DBG_PRINTI(x)
#define DBG_PRINTIN(x)
#define DBG_PRINTP(x)
#define DBG_PRINTPN(x)
#endif // DEBUG

#endif // __ADFS_H__

