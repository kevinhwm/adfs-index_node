/* adfs.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#define ADFS_VERSION		"3.0"
#define ADFS_MAX_FILE_SIZE	0x10000000      // 256MB
#define ADFS_MAX_PATH		1024
#define ADFS_FILENAME_LEN	256
#define ADFS_ZONENAME_LEN	128
#define ADFS_NODENAME_LEN	24
#define ADFS_NAMESPACE_LEN	128
#define ADFS_UUID_LEN		24		// exactly 24 bytes


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


#endif // __ADFS_H__

