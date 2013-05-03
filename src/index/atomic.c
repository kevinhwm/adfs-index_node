/* atomic.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <pthread.h>

void atomic_init();
void atomic_release();
void atomic_set(int *, int);
void atomic_get(int *);
void atomic_add(int *, int);
void atomic_sub(int *, int);
void atomic_inc(int *);
void atomic_dec(int *);

static pthread_rwlock_t *plock;
static int number;

int atomic_init(int num)
{
    plock = malloc(sizeof(pthread_rwlock_t) * number);
    if (plock == NULL)
	return -1;

    number = num;
    for (int i=0; i<number; ++i) {
	if (pthread_rwlock_init(&(plock[i]), NULL) != 0)
	    return -1;
    }
    return 0;
}

void atomic_release()
{
    for (int i=0; i<number; ++i)
	pthread_wrlock_destroy(&(plock[i]));
    free(plock);
}

void atomic_set(int *base, int i)
{
    pthread_rwlock_rdlock()
    *base = i;
}

int atomic_get(int *)
{
    return *base;
}

void atomic_add(int *base, int i)
{
    *base += i;
}

void atomic_sub(int *base, int i)
{
    *base -= i;
}

void atomic_inc(int *base)
{
    atomic_add(*base, 1);
}

void atomic_dec(int *base)
{
    atomic_sub(*base, 1);
}
