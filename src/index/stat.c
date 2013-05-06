/* manager.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <time.h>
#include <string.h>
#include "ai.h"

static void s_release(AIStat *ps);
static int * s_get(AIStat *ps, time_t *);
static void s_inc(AIStat *ps);

ADFS_RESULT ais_init(AIStat *ps, unsigned long stat_start, int min)
{
    if (ps) {
	memset(ps, 0, sizeof(AIStat));
	ps->stat_start = stat_start;
	ps->stat_min = min;
	ps->get = s_get;
	ps->inc = s_inc;
	ps->release = s_release;
	ps->stat_count = malloc(sizeof(int) * ps->stat_min);
	if (ps->stat_count == NULL)
	    return ADFS_ERROR;
	memset(ps->stat_count, 0, sizeof(int) * ps->stat_min);
    }
    return ADFS_OK;
}

static void s_release(AIStat *ps)
{
    free(ps->stat_count);
    ps->stat_count = NULL;
}

static int * s_get(AIStat *ps, time_t *t)
{
    unsigned long t_cur = (unsigned long)*t;
    unsigned long interval = (t_cur - ps->stat_start + 60)/60;
    if (interval >= ps->stat_min)
	memset(ps->stat_count, 0, ps->stat_min);

    int pos_cur = interval % ps->stat_min;
    while (pos_cur != ps->pos_last) {
	__sync_and_and_fetch(ps->stat_count + pos_cur, 0);
	ps->pos_last = (ps->pos_last + 1) % ps->stat_min;
    }
    return ps->stat_count + pos_cur;
}

static void s_inc(AIStat *ps)
{
    unsigned long t_cur;
    t_cur = (unsigned long)time(NULL);
    unsigned long interval = (t_cur - ps->stat_start + 60)/60;
    if (interval >= ps->stat_min)
	memset(ps->stat_count, 0, ps->stat_min);

    int pos_cur = interval % ps->stat_min;
    while (pos_cur != ps->pos_last) {
	__sync_and_and_fetch(ps->stat_count + pos_cur, 0);
	ps->pos_last = (ps->pos_last + 1) % ps->stat_min;
    }
    __sync_add_and_fetch(ps->stat_count + pos_cur, 1);
}

