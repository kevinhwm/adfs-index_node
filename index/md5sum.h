#ifndef MD5SUM_H
#define MD5SUM_H
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef unsigned int uint;
typedef unsigned char byte;
enum
{
	S11=	7,
	S12=	12,
	S13=	17,
	S14=	22,

	S21=	5,
	S22=	9,
	S23=	14,
	S24=	20,

	S31=	4,
	S32=	11,
	S33=	16,
	S34=	23,

	S41=	6,
	S42=	10,
	S43=	15,
	S44=	21
};




typedef struct MD5state
{
	uint len;
	uint state[4];
}MD5state;
MD5state *nil;

int debug;
int hex;    /* print in hex?  (instead of default base64) */
typedef unsigned long ulong;
typedef unsigned char uchar;

static uchar t64d[256];
static char t64e[64];

void encode(byte*, uint*, uint);
void decode(uint*, byte*, uint);
MD5state* md5(byte*, uint, byte*, MD5state*);
char* sum(FILE*, char*);
extern int enc64(char*,byte*,int);
