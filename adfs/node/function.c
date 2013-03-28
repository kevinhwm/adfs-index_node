/*
 * huangtao@antiy.com
 */

#include "an.h"
#include <dirent.h>

int count_kch(char * dir)
{
    int total_count=0;

    DIR* dirp;
    struct dirent* direntp;

    dirp = opendir( dir );
    if( dirp != NULL ) 
    {
        while ((direntp = readdir(dirp)) != NULL)
        {
            if (check_name(direntp->d_name) == ADFS_OK)
                total_count++;
        }

        closedir( dirp );
        if (total_count>0)
            return total_count;
        else
            return 1;
    }
    return 0;
}


ADFS_RESULT check_name(char * name)
{
    if (name == NULL)
        return ADFS_ERROR;

    if (strlen(name) > 8)
        return ADFS_ERROR;

    char *pos = strstr(name, ".kch");
    if (pos == NULL)
        return ADFS_ERROR;

    for (int i=0; i < pos-name; i++)
    {
        if (name[i] < '0' || name[i] > '9')
            return ADFS_ERROR;
    }

    if (strcmp(pos, ".kch") != 0)
        return ADFS_ERROR;

    return ADFS_OK;
}


