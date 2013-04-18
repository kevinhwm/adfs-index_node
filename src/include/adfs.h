/* Antiy Labs. Basic Platform R & D Center
 * adfs.h
 *
 * huangtao@antiy.com
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#define ADFS_VERSION            "3.0"
#define ADFS_MAX_FILE_SIZE      0x10000000      // 256MB
#define ADFS_MAX_PATH           1024

typedef enum _ADFS_RESULT
{
    ADFS_OK         = 0,
    ADFS_ERROR      = -1,
}ADFS_RESULT;



#ifdef DEBUG
#include <stdio.h>

#define DBG_PRINTS(x)   printf("%s", x)
#define DBG_PRINTSN(x)  printf("%s\n", x)
#define DBG_PRINTI(x)   printf("%ld", x)
#define DBG_PRINTIN(x)  printf("%ld\n", x)
#define DBG_PRINTP(x)   printf("%lu", x)
#define DBG_PRINTPN(x)  printf("%lu\n", x)
#else
#define DBG_PRINTS(x)
#define DBG_PRINTSN(x)
#define DBG_PRINTI(x)
#define DBG_PRINTIN(x)
#define DBG_PRINTP(x)
#define DBG_PRINTPN(x)
#endif // DEBUG

#endif // __ADFS_H__

