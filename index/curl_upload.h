/*
 * Description	: libcurl interface for upload
 * Athour	: GavinMa
 * Email	: crackme@antiy.com
 * Date		: 2011-11-24 22:15
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

int upkey(char *url, char *name, char *data);
int upload(char *url, char *, char*, long);
int delete_key(char * url);

