/* ai_record.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <stdlib.h>	// rand
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>	// gettimeofday
#include "ai_record.h"

static void r_release(AIRecord *_this);
static void r_create_uuid(AIRecord *_this);
static ADFS_RESULT r_add(AIRecord *_this, const char *zone, const char *node);
static char * r_get_string(AIRecord *_this);

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

static void r_release(AIRecord *_this)
{
    AIPosition *pp=_this->head, *tmp;
    while (pp) {tmp=pp; pp=pp->next; free(tmp);}
}

static void r_create_uuid(AIRecord *_this)
{
    time_t t;
    struct tm *lt;
    struct timeval tv;
    char buf[32] = {0};

    time(&t);
    lt = localtime(&t);
    gettimeofday(&tv, NULL);
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", lt);
    snprintf(_this->uuid, sizeof(_this->uuid), "_%s%06ld%03d", buf, tv.tv_usec, rand()%1000);
}

static ADFS_RESULT r_add(AIRecord *_this, const char *zone, const char *node)
{
    AIPosition *pp = malloc(sizeof(AIPosition));
    if (pp == NULL) {return ADFS_ERROR;}
    _this->num++;
    snprintf(pp->zone_node, sizeof(pp->zone_node), "%s#%s", zone, node);

    pp->prev = _this->tail;
    pp->next = NULL;
    if (_this->tail) {_this->tail->next = pp;}
    else {_this->head = pp;}
    _this->tail = pp;
    return ADFS_OK;
}

static char * r_get_string(AIRecord *_this)
{
    int len = ADFS_UUID_LEN + _this->num * sizeof(_this->head->zone_node);
    char *record = malloc(len);
    if (record == NULL) {return NULL;}
    memset(record, 0, len);
    strcpy(record, _this->uuid);

    AIPosition *pp = _this->head;
    while (pp) {
	strcat(record, pp->zone_node);
	strcat(record, "|");
	pp = pp->next;
    }
    // set the last "|" to be "\0"
    record[strlen(record)-1] = '\0';
    return record;
}

