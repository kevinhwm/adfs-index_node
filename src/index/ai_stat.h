/* ai_stat.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __STAT_H__
#define __STAT_H__

#include <time.h>
#include "../include/adfs.h"

typedef struct AIStat 
{
    unsigned long start;
    unsigned long pos_last; 	// last record 
    unsigned long scope;		// in minutes
    int *count;

    void (*release)(struct AIStat *);
    int *(*get)(struct AIStat *, time_t *);
    void (*inc)(struct AIStat *);
}AIStat;

ADFS_RESULT stat_init(AIStat *ps, unsigned long start, int min);

#endif // __STAT_H__
