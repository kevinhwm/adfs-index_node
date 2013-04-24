/* Antiy Labs. Basic Platform R & D Center
 * def.h
 *
 * huangtao@antiy.com
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#define ADFS_VERSION		"3.0"
#define ADFS_MAX_FILE_SIZE	0x10000000      // 256MB
#define ADFS_MAX_PATH		2048
#define ADFS_URL_PATH		1024

typedef enum ADFS_RESULT
{
    ADFS_ERROR	= -1,
    ADFS_OK	= 0,
}ADFS_RESULT;


#ifdef DEBUG
#include <stdio.h>

#define DBG_PRINTS(x)	printf("%s", x)
#define DBG_PRINTSN(x)	printf("%s\n", x)
#define DBG_PRINTI(x)	printf("%ld", x)
#define DBG_PRINTIN(x)	printf("%ld\n", x)
#define DBG_PRINTP(x)	printf("%lu", x)
#define DBG_PRINTPN(x)	printf("%lu\n", x)
#else
#define DBG_PRINTS(x)
#define DBG_PRINTSN(x)
#define DBG_PRINTI(x)
#define DBG_PRINTIN(x)
#define DBG_PRINTP(x)
#define DBG_PRINTPN(x)
#endif // DEBUG

// message list
#define MSG_OK			"OK."
#define MSG_FAIL_LONG_URL	"Failed. File name is too long, must less than 1024."
#define MSG_FAIL_ILLEGAL_NAME	"Failed. File name is illegal."
#define MSG_FAIL_NO_FILE	"Failed. No file."

#endif // __ADFS_H__

