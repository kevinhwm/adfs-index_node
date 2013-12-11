/* namespace.c
 *
 * kevinhwm@gmail.com
 */

#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <kclangc.h>
#include "manager.h"
#include "namespace.h"


static int ns_release(CINameSpace *_this);
static int ns_output(CINameSpace *_this, const char *name, const char *info);

static int open_log(CINsPrim *pprim, const char *name_space);
static int close_log();
static int scan_fin(const char * dir);
static int get_fileid(char * name);

extern CIManager g_manager;


int GIns_init(CINameSpace *_this, const char *name, const char *db_args, int primary)
{
    memset(_this, 0, sizeof(CINameSpace));
    strncpy(_this->name, name, sizeof(_this->name));
    _this->index_db = kcdbnew();
    if (kcdbopen(_this->index_db, db_args, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK) == 0) { return -1; }

    if (primary) {
	_this->prim = malloc(sizeof(CINsPrim));
	if (_this->prim == NULL) { return -1; }
	memset(_this->prim, 0, sizeof(CINsPrim));

	pthread_mutex_init(&_this->prim->lock, NULL);
	char buf[_DFS_MAX_LEN] = {0};
	sprintf(buf, "%s/%s", g_manager.syn_dir, name);
	if (scan_fin(buf) < 0) { free(_this->prim); return -1; }

	_this->output = ns_output;
    }
    else {
	_this->output = NULL;
    }

    _this->release = ns_release;
    return 0;
}

static int ns_release(CINameSpace *_this)
{
    DBG_PRINTS("ns release 1\n");
    if (_this->index_db) { 
	kcdbclose(_this->index_db);
	kcdbdel(_this->index_db);
	_this->index_db = NULL; 
    }
    DBG_PRINTS("ns release 2\n");
    if (_this->prim) {
	pthread_mutex_lock(&_this->prim->lock);
	close_log(_this->prim);
	pthread_mutex_destroy(&_this->prim->lock);
	free(_this->prim);
	_this->prim = NULL;
    }
    DBG_PRINTS("ns release 3\n");
    return 0;
}

static int ns_output(CINameSpace *_this, const char *name, const char *info)
{
    pthread_mutex_lock(&_this->prim->lock);

#ifdef DEBUG
    if (_this->prim->f_inc) { 
	fprintf(stderr, "file handle is not NULL\n"); 
	fprintf(stderr, "file name %s\n", _this->prim->f_name); 
    }
    else { 
	fprintf(stderr, "file handle is NULL\n"); 
	fprintf(stderr, "file name %s\n", _this->prim->f_name); 
    }
#endif

    if (open_log(_this->prim, _this->name) < 0) { 
	pthread_mutex_unlock(&_this->prim->lock);
	return -1; 
    }
    fprintf(_this->prim->f_inc, "%s\t%s\n", name, info);
    fflush(_this->prim->f_inc);

#ifdef DEBUG
    if (_this->prim->f_inc) { 
	fprintf(stderr, "file handle is not NULL\n"); 
	fprintf(stderr, "file name %s\n", _this->prim->f_name); 
    }
    else { 
	fprintf(stderr, "file handle is NULL\n"); 
	fprintf(stderr, "file name %s\n", _this->prim->f_name); 
    }
#endif

    pthread_mutex_unlock(&_this->prim->lock);
    return 0;
}

static int open_log(CINsPrim *pprim, const char *name_space)
{
    if (pprim->f_inc) { return 0; }

    char buf[_DFS_MAX_LEN] = {0};
    sprintf(buf, "%s/%s/%d", g_manager.syn_dir, name_space, pprim->num);
    pprim->f_inc = fopen(buf, "a");
    if (pprim->f_inc == NULL) { return -1; }
    strncpy(pprim->f_name, buf, sizeof(pprim->f_name));
    return 0;
}

static int close_log(CINsPrim *pprim)
{
    DBG_PRINTS("ns close log 1\n");
    if (pprim->f_inc) {
	DBG_PRINTS("ns close log 2\n");
	fclose(pprim->f_inc);
	DBG_PRINTS("ns close log 3\n");
	pprim->f_inc = NULL;

	char buf[_DFS_MAX_LEN] = {0};
	sprintf(buf, "%s.fin", pprim->f_name);

	DBG_PRINTS("ns close log 4\n");
	DBG_PRINTS(pprim->f_name);
	DBG_PRINTS(buf);

	if (rename(pprim->f_name, buf) < 0) { fprintf(stderr, "rename file error\n"); }
	memset(pprim->f_name, 0, sizeof(pprim->f_name));
	pprim->num++;
    }
    DBG_PRINTS("ns close log 5\n");
    return 0;
}

static int scan_fin(const char * dir)
{
    int max_id = 0;
    DIR* dp;
    struct dirent* dirp;

    dp = opendir(dir);
    if (dp != NULL) {
	while ((dirp = readdir(dp)) != NULL) {
	    int id = get_fileid(dirp->d_name);
	    max_id = id > max_id ? id : max_id;
	}
	closedir(dp);
    }
    else {
	if (mkdir(dir, 0755) < 0) { return -1; }
    }
    return max_id;
}

#define _DFS_INC_ID_MAX		16

static int get_fileid(char * name)
{
    if (name == NULL) { return -1; }
    if (strlen(name) > _DFS_INC_ID_MAX) { return -1; }
    char *pos = strstr(name, ".fin");
    if (pos == NULL) { return -1; }
    if (strcmp(pos, ".fin") != 0) { return -1; }

    char tmp[_DFS_INC_ID_MAX] = {0};
    strncpy(tmp, name, pos-name);

    for (unsigned int i=0; i<strlen(tmp); ++i) {
	if ( !isdigit(name[i]) ) { return -1; }
    }
    return atoi(tmp);
}

