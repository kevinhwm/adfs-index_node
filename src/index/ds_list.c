/* Antiiy Labs. Basic Platform R & D Center
 * ds_list.c
 *
 * huangtao@antiy.com
 */

#include "ai.h"

ADFS_RESULT list_add(DS_List **dsl, const char *s, size_t len)
{
    DS_List * new_dsl = (DS_List *)malloc(sizeof(DS_List));
    if (new_dsl == NULL)
        return ADFS_ERROR;
    memset(new_dsl, 0, sizeof(DS_List));

    strncpy(new_dsl->data, s, sizeof(new_dsl->data));
    new_dsl->next = NULL;

    if (*dsl == NULL)
        *dsl = new_dsl;
    else
    {
        DS_List * tail = *dsl;
        while (tail->next)
            tail = tail->next;
        tail->next = new_dsl;
    }

    return ADFS_OK;
}

void list_free(DS_List *dsl)
{
    DS_List *tmp; 
    DS_List *rest = dsl;

    while (rest)
    {
        tmp = rest;
        rest = rest->next;
        free(tmp);
    }
}
