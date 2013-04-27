/* record.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "record.h"

void r_create_uuid(AIRecord *);
ADFS_RESULT r_add(AIRecord *, const char *, const char *);
void r_release(AIRecord *);
char * r_get_string(AIRecord *);

void air_init(AIRecord *pr)
{
    if (pr) {
	memset(pr, 0, sizeof(AIRecord));
	pr->add = r_add;
	pr->release = r_release;
	pr->get_string = r_get_string;
	r_create_uuid(pr);
    }
}

void r_create_uuid(AIRecord *_this)
{
    time_t t;
    struct tm *lt;
    struct timeval tv;
    char buf[32] = {0};

    time(&t);
    lt = localtime(&t);
    gettimeofday(&tv, NULL);
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", lt);
    sprintf(_this->uuid, "%s%06ld%04d", buf, tv.tv_usec, rand()%10000);
}

ADFS_RESULT r_add(AIRecord *_this, const char *zone, const char *node)
{
    AIPosition *pp = malloc(sizeof(AIPosition));
    if (pp == NULL)
	return ADFS_ERROR;

    _this->num++;
    sprintf(pp->zone_node, "%s#%s", zone, node);
    pp->pre = _this->tail;
    pp->next = NULL;
    if (_this->tail)
	_this->tail->next = pp;
    else
	_this->head = pp;
    _this->tail = pp;
    return ADFS_OK;
}

void r_release(AIRecord *_this)
{
    AIPosition *pp = _this->head;
    while (pp) {
	AIPosition *tmp = pp;
	pp = pp->next;
	free(tmp);
    }
}

char * r_get_string(AIRecord *_this)
{
    DBG_PRINTSN("record 1");
    int len = ADFS_UUID_LEN + _this->num * sizeof(_this->head->zone_node);
    char *record = malloc(len);
    if (record == NULL)
	return NULL;
    DBG_PRINTSN("record 10");
    memset(record, 0, len);
    strcpy(record, _this->uuid);

    DBG_PRINTSN("record 20");
    AIPosition *pp = _this->head;
    while (pp) {
	strcat(record, pp->zone_node);
	strcat(record, "|");
	pp = pp->next;
    }
    DBG_PRINTSN("record 30");
    // set the last "|" to be "\0"
    record[strlen(record)-1] = '\0';
    DBG_PRINTSN("record 40");
    return record;
}

