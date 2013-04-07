#include <stdio.h>
#include <string.h>
#include "an.h"


static int parse(char *p, char *key, char *value);


/*
 * example
 * char buf[2048] = {0};
 * get_conf("a.conf", "name", buf, sizeof(buf));
 * printf("[%d]%s\n", strlen(buf), buf);
 */


int get_conf(const char * pfile, const char * target, char *value, size_t len)
{
    char buf[2048] = {0};
    char key[2048] = {0};
    char val[2048] = {0};
    int res = 0;
    
    FILE * f = fopen(pfile, "r");

    while (fgets(buf, sizeof(buf), f))
    {
        memset(key, 0, sizeof(key));
        memset(val, 0, sizeof(val));
        if (parse(buf, key, val))
        {
            if (!strcmp(target, key))
            {
                if (strlen(val) >= len)
                {
                    res = 0;
                    break;
                }
                else
                {
                    strncpy(value, val, len);
                    res = 1;
                    break;
                }
            }
        }
    }

    fclose(f);
    return res;
}

static int parse(char *line, char *key, char *value)
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

