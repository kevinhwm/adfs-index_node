/* ai_stat.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "ai_stat.h"

static void s_release(AIStat *ps);
static int * s_get(AIStat *ps, time_t *);
static void s_inc(AIStat *ps);

ADFS_RESULT stat_init(AIStat *ps, unsigned long stat_start, int minutes)
{
    if (ps) {
	memset(ps, 0, sizeof(AIStat));
	ps->start = stat_start;
	ps->scope = minutes;
	ps->get = s_get;
	ps->inc = s_inc;
	ps->release = s_release;
	ps->count = malloc(sizeof(int) * ps->scope);
	if (ps->count == NULL) {return ADFS_ERROR;}
	memset(ps->count, 0, sizeof(int) * ps->scope);
    }
    return ADFS_OK;
}

static void s_release(AIStat *ps) {if(ps && ps->count) {free(ps->count); ps->count = NULL;}}

static int * s_get(AIStat *ps, time_t *t)
{
    unsigned long t_cur, pos_cur, interval;

    t_cur = (unsigned long)*t;
    interval = (t_cur - ps->start + 60)/60;
    if (interval >= ps->scope) {memset(ps->count, 0, ps->scope);}
    pos_cur = interval % ps->scope;
    while (pos_cur != ps->pos_last) {
	__sync_and_and_fetch(ps->count + ps->pos_last, 0);
	ps->pos_last = (ps->pos_last + 1) % ps->scope;
    }
    return ps->count + pos_cur;
}

static void s_inc(AIStat *ps)
{
    unsigned long t_cur, interval, pos_cur;

    t_cur = (unsigned long)time(NULL);
    interval = (t_cur - ps->start + 60)/60;
    if (interval >= ps->scope) {memset(ps->count, 0, ps->scope);}
    pos_cur = interval % ps->scope;
    while (pos_cur != ps->pos_last) {
	__sync_and_and_fetch(ps->count + ps->pos_last, 0);
	ps->pos_last = (ps->pos_last + 1) % ps->scope;
    }
    __sync_add_and_fetch(ps->count + pos_cur, 1);
}

