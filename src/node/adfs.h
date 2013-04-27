/* Antiy Labs. Basic Platform R & D Center
 * def.h
 *
 * huangtao@antiy.com
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


/////////////////////////////////////////////////////////////////////////////////
//				message list
/////////////////////////////////////////////////////////////////////////////////

// OK
#define MSG_OK			"OK."

/////////////////////////////////////////////////////////////////////////////////
// manager 
#define MSG_FAIL_LONG_PATH	"Failed. Database path is too long. It must be less than 1024."
#define MSG_FAIL_NO_PATH	"Failed. Database path dose not exist. Please create it yourself."
#define MSG_FAIL_OPEN_DB	"Failed. Can not create or open database."
#define MSG_FAIL_CONFIG		"Failed. Something in config file are illegal. Please check."
#define MSG_FAIL_NAMESPACE	"Failed. Namespace is invalid."
#define MSG_FAIL_MALLOC		"Failed. Can not malloc"

// interface
#define MSG_FAIL_LONG_URL	"Failed. File name is too long, must less than 1024."
#define MSG_FAIL_ILLEGAL_NAME	"Failed. File name is illegal."
#define MSG_FAIL_NO_FILE	"Failed. No file."

#endif // __ADFS_H__
