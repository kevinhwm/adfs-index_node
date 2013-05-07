/* log.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "adfs.h"

extern int g_log_level;
static char log_file[256] = {0};
static FILE *f_log = NULL;
static pthread_mutex_t mutex;

int log_init(const char *filename)
{
    if (strlen(filename) > 256)
	return -1;
    strncpy(log_file, filename, sizeof(log_file));
    f_log = fopen(log_file, "a");
    if (f_log == NULL)
	return -1;
    fprintf(f_log, "\n");
    fflush(f_log);

    if (pthread_mutex_init(&mutex, NULL) != 0) {
	fclose(f_log);
	return -1;
    }
    return 0;
}

void log_release()
{
    pthread_mutex_destroy(&mutex);
    fflush(f_log);
    fclose(f_log);
}

void log_out(const char *module, const char *info, LOG_LEVEL level)
{
    time_t t = time(NULL);
    struct tm *lt;
    char stime[32] = {0};

    if (level < 0 || level > 5)
	return ;
    else if (level > g_log_level)
	return ;

    lt = localtime(&t);
    strftime(stime, sizeof(stime), "%Y-%m-%d %H:%M:%S", lt);

    if (pthread_mutex_lock(&mutex) != 0)
	return ;

    fprintf(f_log, "%s ", stime);
    switch (level) {
	case 0:
	    fprintf(f_log, "fatal "); break;
	case 1:
	    fprintf(f_log, "error "); break;
	case 2:
	    fprintf(f_log, "warn "); break;
	case 3:
	    fprintf(f_log, "info "); break;
	case 4:
	    fprintf(f_log, "debug "); break;
    }
    fprintf(f_log, "%s ", module);
    fprintf(f_log, "%s ", info);
    fprintf(f_log, "\n");
    fflush(f_log);

    pthread_mutex_unlock(&mutex);
}

