/* def.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __DEF_H__
#define __DEF_H__

#define _DFS_VERSION		"3.3.14"

#define _DFS_MAX_LEN		1024
#define _DFS_FILENAME_LEN	512
#define _DFS_ZONENAME_LEN	128
#define _DFS_NAMESPACE_LEN	128
#define _DFS_NODENAME_LEN	128
#define _DFS_NODE_ID_LEN	12		// <num>.kch : 8+4=12
#define _DFS_UUID_LEN		24		// exactly 24 bytes

#define _DFS_RUNNING_FLAG	"running.flag"

#include "cJSON.h"
#include "log.h"


typedef enum {
    S_NA		= 0,
    S_READ_ONLY,
    S_READ_WRITE
}_DFS_NODE_STATE;

//==============================================================================

// function.c
int get_filename_from_url(char * p, char *pattern);

// conf.c
cJSON * conf_parse(const char *conf_file);
void conf_release(cJSON *json);

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

#endif // __DEF_H__

