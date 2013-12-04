/* log.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include "log.h"


static struct LogFile *hlog = NULL;

int log_init(LOG_LEVEL level, const char *instance_id)
{
    time_t t;
    struct tm lt;
    char buf[64] = {0};
    DIR *dirp = NULL;

    hlog = malloc(sizeof(struct LogFile));
    if (hlog == NULL) { return -1; }
    memset(hlog, 0, sizeof(struct LogFile));

    // level
    if (level < LOG_LEVEL_SYSTEM || level >= LOG_LEVEL_ALL) { goto err1; }
    hlog->level = level;

    // lock
    hlog->lock = malloc(sizeof(pthread_mutex_t));
    if (hlog->lock == NULL) { goto err1; }
    if (pthread_mutex_init(hlog->lock, NULL) != 0) { goto err2; }

    // instance_id
    if (instance_id == NULL || strlen(instance_id) == 0) {
	time(&t);
	localtime_r(&t, &lt);
	strftime(buf, sizeof(buf), "%y%m%d", &lt);
	snprintf(hlog->instance_id, sizeof(hlog->instance_id), "ID%s%04d", buf, rand()%10000);
    }
    else { strncpy(hlog->instance_id, instance_id, sizeof(hlog->instance_id)); }

    // log_dir
    strncpy(hlog->log_dir, "log", sizeof(hlog->log_dir));
    dirp = opendir(hlog->log_dir);
    if (dirp == NULL) {
	if (mkdir(hlog->log_dir, 0755) != 0) { goto err2; }
    }
    else { closedir(dirp); }

    return 0;
err2:
    free(hlog->lock);
    hlog->lock = NULL;
err1:
    free(hlog);
    hlog = NULL;
    return -1;
}

void log_release()
{
    if (hlog == NULL) { return ; }
    if (pthread_mutex_lock(hlog->lock) != 0) {return ;}
    if (hlog->flog) { 
	fclose(hlog->flog); 
	hlog->flog = NULL;
    }
    pthread_mutex_unlock(hlog->lock);

    pthread_mutex_destroy(hlog->lock); 
    free(hlog->lock);
    hlog->lock = NULL;

    free(hlog);
    hlog = NULL;
    return ;
}

void log_out(const char *module, const char *info, LOG_LEVEL level)
{
    time_t t;
    struct tm lt;
    char stime[64] = {0};
    char fname[64] = {0};

    if (level < LOG_LEVEL_SYSTEM || level >= LOG_LEVEL_ALL) { return ; }
    else if (level > hlog->level) { return ; }
    if (hlog == NULL) { return ; }

    if (pthread_mutex_lock(hlog->lock) != 0) {return ;}
    time(&t);
    localtime_r(&t, &lt);
    strftime(fname, sizeof(fname), "log/%Y%m%d.log", &lt);
    strftime(stime, sizeof(stime), "%Y-%m-%d %H:%M:%S", &lt);

    if (hlog->flog == NULL) { 
	hlog->flog = fopen(fname, "a");
	if (hlog->flog == NULL) { return ; }
	strncpy(hlog->fname, fname, sizeof(hlog->fname));
	fprintf(hlog->flog, "\n");
    }
    else if (strcmp(fname, hlog->fname)) {
	fclose(hlog->flog);
	hlog->flog = fopen(fname, "a");
	if (hlog->flog == NULL) { return ; }
	strncpy(hlog->fname, fname, sizeof(hlog->fname));
	fprintf(hlog->flog, "\n");
    }

    fprintf(hlog->flog, "%s\t", stime);
    fprintf(hlog->flog, "%s\t", hlog->instance_id);
    switch (level) {
	case 0:  fprintf(hlog->flog, "L0-system\t"); break;
	case 1:  fprintf(hlog->flog, "L1--fatal\t"); break;
	case 2:  fprintf(hlog->flog, "L2--error\t"); break;
	case 3:  fprintf(hlog->flog, "L3---warn\t"); break;
	case 4:  fprintf(hlog->flog, "L4---info\t"); break;
	case 5:  fprintf(hlog->flog, "L5--debug\t"); break;
	default: fprintf(hlog->flog, "LA----all\t"); break;
    }
    fprintf(hlog->flog, "M-%s\t", module);
    fprintf(hlog->flog, "%s\n", info);
    fflush(hlog->flog);
    pthread_mutex_unlock(hlog->lock);
}

