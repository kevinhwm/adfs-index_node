/* conf.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "adfs.h"

static void trim_left_white(char * p);
static void trim_right_white(char * p);

/* example:
 *
 * char buf[ADFS_FILENAME_LEN] = {0};
 * conf_read("a.conf", "name", buf, sizeof(buf));
 * printf("[%d]%s\n", strlen(buf), buf);
 */

ADFS_RESULT conf_read(const char * pfile, const char * target, char *value, size_t len)
{
    char buf[ADFS_FILENAME_LEN] = {0};
    char key[ADFS_FILENAME_LEN] = {0};
    char val[ADFS_FILENAME_LEN] = {0};
    int res = ADFS_ERROR;
    FILE * f = fopen(pfile, "r");
    if (f == NULL)
	return res;

    while (fgets(buf, sizeof(buf), f))
    {
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));
        if (!conf_split(buf, key, val))
            continue;
        if (strcmp(target, key))
            continue;

	int val_len = strlen(val);
        if (val_len == 0 || val_len >= len) {
            break;
        }
        else {
            strncpy(value, val, len);
            res = ADFS_OK;
            break;
        }
    }
    fclose(f);
    return res;
}

// parse config file, get the value of specified key
int conf_split(char *line, char *key, char *value)
{
    char *pos, *tmp;
    trim_left_white(line);
    if (line[0] == '#')
        return 0;
    if ( !(pos = strstr(line, "=")) )
        return 0;
    else if ( (tmp = strstr(pos+1, "=")) )
        return 0;
    strncpy(key, line, pos-line);
    strncpy(value, pos+1, strlen(pos+1));
    trim_right_white(key);
    if (strlen(key) == 0)
        return 0;
    trim_left_white(value);
    trim_right_white(value);
    return 1;
}

int get_filename_from_url(char * p)
{
    if (p == NULL)
	return -1;

    char *pos = strstr(p, "?");
    if (pos != NULL)
        pos[0] = '\0';

    int len = 0;
    len = strlen(p);
    if (len == 0)
        return -1;

    if (p[len-1] == '/')
        p[len-1] = '\0';

    len = strlen(p);
    if (len == 0)
        return -1;

    if (p[0] == '/')
        for (int i=1; i<=len; ++i)
            p[i-1] = p[i];
    if (strlen(p) <= 0)
	return -1;

    return 0;
}

static void trim_left_white(char * p)
{
    int i=0, j=0;
    if (p == NULL)
	return;

    for (; i<strlen(p); ++i) {
        if (isspace(p[i]))
            continue;
        else
            break;
    }

    for (; i<=strlen(p); ++j, ++i)
        p[j] = p[i];
}

static void trim_right_white(char * p)
{
    if (p == NULL)
	return ;

    int i = strlen(p)-1;
    for (; i>=0; --i) {
        if (isspace(p[i]))
            p[i] = '\0';
        else
            break;
    }
}

