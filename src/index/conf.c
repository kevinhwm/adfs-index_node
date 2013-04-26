/*
 * conf.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "ai.h"

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

    while (fgets(buf, sizeof(buf), f))
    {
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));
        if (!conf_split(buf, key, val))
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

// parse config file, then get the value of specified key
int conf_split(char *line, char *key, char *value)
{
    trim_left_white(line);
    if (line[0] == '#')
        return 0;

    char *pos, *tmp;
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

