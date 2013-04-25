/* Antiy Labs. Basic Platform R & D Center
 * ai_function.c
 *
 * huangtao@antiy.com
 */

#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "ai.h"


void trim_left_white(char * p)
{
    int i=0, j=0;
    if (p == NULL)
	return;

    for (; i<strlen(p); ++i)
    {
        if (p[i] == ' ' || p[i] == '\t' || p[i] == '\n' )
            continue;
        else
            break;
    }

    for (; i<=strlen(p); ++j, ++i)
        p[j] = p[i];
}

void trim_right_white(char * p)
{
    if (p == NULL)
	return ;

    int i = strlen(p)-1;
    for (; i>=0; --i)
    {
        if (p[i] == ' ' || p[i] == '\t' || p[i] == '\n' )
            p[i] = '\0';
        else
            break;
    }
}

// parse url, then get real file name.
ADFS_RESULT get_filename_from_url(char * p)
{
    if (p == NULL)
	return ADFS_ERROR;

    char *pos = strstr(p, "?");
    if (pos != NULL)
        pos[0] = '\0';

    int len = 0;
    len = strlen(p);
    if (len == 0)
        return ADFS_ERROR;

    if (p[len-1] == '/')
        p[len-1] = '\0';

    len = strlen(p);
    if (len == 0)
        return ADFS_ERROR;

    if (p[0] == '/')
        for (int i=1; i<=len; ++i)
            p[i-1] = p[i];

    return ADFS_OK;
}

ADFS_RESULT create_time_string(char *buf, size_t len)
{
    if (len < 32)
	return ADFS_ERROR;

    time_t t;
    struct tm *lt;
    struct timeval tv;
    char dt[32] = {0};

    time(&t);
    lt = localtime(&t);
    gettimeofday(&tv, NULL);
    strftime(dt, sizeof(dt), "%Y%m%d%H%M%S", lt);

    memset(buf, 0, len);
    sprintf(buf, "%s%06d%04d", dt, tv.tv_usec, rand()%1000);
    return ADFS_OK;
}

