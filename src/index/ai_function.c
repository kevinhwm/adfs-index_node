#include <stdio.h>
#include <string.h>
#include "ai.h"


/*
 * example:
 *
 * char buf[NAME_MAX] = {0};
 * get_conf("a.conf", "name", buf, sizeof(buf));
 * printf("[%d]%s\n", strlen(buf), buf);
 */

ADFS_RESULT get_conf(const char * pfile, const char * target, char *value, size_t len)
{
    char buf[NAME_MAX] = {0};
    char key[NAME_MAX] = {0};
    char val[NAME_MAX] = {0};
    int res = ADFS_ERROR;
    
    FILE * f = fopen(pfile, "r");

    while (fgets(buf, sizeof(buf), f))
    {
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));
        if (!parse_conf(buf, key, val))
            continue;
        if (strcmp(target, key))
            continue;

        if (strlen(val) >= len)
        {
            break;
        }
        else
        {
            strncpy(value, val, len);
            res = ADFS_OK;
            break;
        }
    }

    fclose(f);
    return res;
}

void trim_left(char * p)
{
    int i=0, j=0;
    for (; i<strlen(p); ++i)
    {
        if (p[i] == ' ' || p[i] == '\t' || p[i] == '\n' )
            continue;
        else
            break;
    }
    for (; i<=strlen(p); ++j, ++i)
    {
        p[j] = p[i];
    }
}

void trim_right(char * p)
{
    int i = strlen(p)-1;
    for (; i>=0; --i)
    {
        if (p[i] == ' ' || p[i] == '\t' || p[i] == '\n' )
            p[i] = '\0';
        else
            break;
    }
}

// parse config file, then get the value of specified key
int parse_conf(char *line, char *key, char *value)
{
    trim_left(line);
    if (line[0] == '#')
        return 0;

    char *pos, *tmp;
    if ( !(pos = strstr(line, "=")) )
        return 0;
    else if ( (tmp = strstr(pos+1, "=")) )
        return 0;

    strncpy(key, line, pos-line);
    strncpy(value, pos+1, strlen(pos+1));

    trim_right(key);
    if (strlen(key) == 0)
        return 0;
    trim_left(value);
    trim_right(value);

    return 1;
}

// parse url, then get real file name.
ADFS_RESULT parse_filename(char * p)
{
    char *pos = strstr(p, "?");
    if (pos != NULL)
        pos[0] = '\0';

    DBG_PRINTS("parse file name 10\n");
    DBG_PRINTSN(p);
    int len = 0;
    len = strlen(p);
    if (len == 0)
        return ADFS_ERROR;

    DBG_PRINTS("parse file name 20\n");
    DBG_PRINTSN(p);
    if (p[len-1] == '/')
        p[len-1] = '\0';

    len = strlen(p);
    if (len == 0)
        return ADFS_ERROR;

    DBG_PRINTS("parse file name 30\n");
    DBG_PRINTSN(p);
    if (p[0] == '/')
        for (int i=1; i<=len; ++i)
            p[i-1] = p[i];

    DBG_PRINTS("parse file name 40\n");
    DBG_PRINTSN(p);
    return ADFS_OK;
}

