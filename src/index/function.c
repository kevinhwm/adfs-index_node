/* function.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <string.h>

void trim_left_white(char * p)
{
    int i=0, j=0;
    if (p == NULL)
	return;

    for (; i<strlen(p); ++i) {
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
    for (; i>=0; --i) {
        if (p[i] == ' ' || p[i] == '\t' || p[i] == '\n' )
            p[i] = '\0';
        else
            break;
    }
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

