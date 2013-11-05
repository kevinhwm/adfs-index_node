// log.c
// kevinhwm@gmail.com

#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <pthread.h>

typedef enum {
    LOG_LEVEL_SYSTEM	= 0,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_ALL
}LOG_LEVEL;

typedef struct LogFile
{
    LOG_LEVEL 		level;
    pthread_mutex_t *	lock;
    char 		instance_id[64];
    char 		log_dir[64];
    char 		fname[64];
    FILE * 		flog;
}LogFile;

int log_init(LOG_LEVEL level);
void log_release();
void log_out(const char *module, const char *info, LOG_LEVEL level);

#endif // __LOG_H__

