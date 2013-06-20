/* Antiy Labs. Basic Platform R & D Center.
 * an_manager.c
 *
 * huangtao@antiy.com
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kclangc.h>

#include "an_namespace.h"
#include "an_manager.h"
#include "../include/adfs.h"

static ANNameSpace * m_create_ns(const char *name_space);
static ANNameSpace * m_get_ns(const char * name_space);
static ADFS_RESULT m_init_log(const char *conf_file);
static int m_scan_kch(const char * dir);
static int m_get_fileid(char * name);

ADFS_RESULT conf_read(const char * pfile, const char * target, char *value, size_t len);	// in conf.c

ANManager g_manager;
LOG_LEVEL g_log_level = LOG_LEVEL_DEBUG;
int g_another_running = 0;

ADFS_RESULT anm_init(const char * conf_file, const char *path, unsigned long mem_size) 
{
    char msg[1024] = {0};

    ANManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));
    pthread_rwlock_init(&pm->ns_lock, NULL);
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;
    strncpy(pm->path, path, sizeof(pm->path));

    DIR *dirp = opendir(path);
    if( dirp == NULL ) {
	snprintf(msg, sizeof(msg), "[%s]->path error", path);
	log_out("manager", msg, LOG_LEVEL_ERROR);
        return ADFS_ERROR;
    }
    closedir(dirp);
    char f_flag[1024] = {0};
    snprintf(f_flag, sizeof(f_flag), "%s/adfs.flag", path);
    if (access(f_flag, F_OK) != -1) {
	snprintf(msg, sizeof(msg), "[%s]->Another instance is running.", f_flag);
	log_out("manager", msg, LOG_LEVEL_SYSTEM);
	g_another_running = 1;
	return ADFS_ERROR;
    }
    else {
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);
	FILE *f = fopen(f_flag, "wb+");
	fprintf(f, "%s", asctime(lt));
	fclose(f);
    }

    struct dirent *ent;
    dirp = opendir(path);
    while ((ent = readdir(dirp)) != NULL) {
	if (ent->d_type == 4 && ent->d_name[0] != '.') {m_create_ns(ent->d_name);}
    }
    closedir(dirp);

    snprintf(msg, sizeof(msg), "[%s]->init db path", path);
    log_out("manager", msg, LOG_LEVEL_INFO);
    if (m_init_log(conf_file) == ADFS_ERROR) {return ADFS_ERROR;}
    return ADFS_OK;
}

void anm_exit() 
{
    ANManager * pm = &g_manager;
    pthread_rwlock_destroy(&pm->ns_lock);
    ANNameSpace * pns = pm->head;
    while (pns) {
	ANNameSpace *tmp = pns;
	pns = pns->next;
	kcdbclose(tmp->index_db);
	kcdbdel(tmp->index_db);
	tmp->release(tmp);
	free(tmp);
    }
    log_release();
    if (!g_another_running) {
	char f_flag[1024] = {0};
	snprintf(f_flag, sizeof(f_flag), "%s/adfs.flag", pm->path);
	remove(f_flag);
    }
}

ADFS_RESULT anm_save(const char * ns, const char *fname, size_t fname_len, void * fdata, size_t fdata_len)
{
    ANManager *pm = &g_manager;
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL && (pns = m_create_ns(name_space)) == NULL) {return ADFS_ERROR;}
    if (pns->needto_split(pns)) {
	char db_args[ADFS_MAX_PATH] = {0};
	snprintf(db_args, sizeof(db_args), "#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", pm->kc_apow, pm->kc_fbp, pm->kc_bnum*3, pm->kc_msiz);
	if (pns->split_db(pns, pm->path, db_args) == ADFS_ERROR) {return ADFS_ERROR;}
    }
    if (!kcdbset(pns->tail->db, fname, fname_len, fdata, fdata_len)) {return ADFS_ERROR;}
    pns->count_add(pns);
    char buf[16] = {0};
    sprintf(buf, "%d", pns->tail->id);
    if (!kcdbset(pns->index_db, fname, fname_len, buf, strlen(buf))) {return ADFS_ERROR;}
    return ADFS_OK;
}

void anm_get(const char *ns, const char *fname, void ** ppfile_data, size_t *pfile_size)
{
    *ppfile_data = NULL;
    *pfile_size = 0;
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL) {return ;}

    size_t len = 0;
    char *id = kcdbget(pns->index_db, fname, strlen(fname), &len);
    if (id == NULL) {return ;}
    NodeDB *pn = pns->get(pns, atoi(id));
    if (pn == NULL) {kcfree(id); return ;}
    *ppfile_data = kcdbget(pn->db, fname, strlen(fname), pfile_size);
    kcfree(id);
}

ADFS_RESULT anm_erase(const char *ns, const char *fname)
{
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL) {return ADFS_OK;}
    if ( kcdbremove(pns->index_db, fname, strlen(fname)) ) {return ADFS_OK;}
    else {return ADFS_ERROR;}
}

ADFS_RESULT anm_syn()
{
    ADFS_RESULT res = ADFS_OK;
    ANManager * pm = &g_manager;
    ANNameSpace * pns = pm->head;
    while (pns) {
	pns->syn(pns);
	pns = pns->next;
    }
    return res;
}

////////////////////////////////////////////////////////////////////////////////
//private
// private
static ANNameSpace * m_get_ns(const char * name_space)
{
    pthread_rwlock_rdlock(&g_manager.ns_lock);
    ANNameSpace * tmp = g_manager.head;
    while (tmp) {
	if (strcmp(tmp->name, name_space) == 0) { pthread_rwlock_unlock(&g_manager.ns_lock); return tmp; }
	tmp = tmp->next;
    }
    pthread_rwlock_unlock(&g_manager.ns_lock);
    return NULL;
}

static ANNameSpace * m_create_ns(const char *name_space)
{
    if (strlen(name_space) >= ADFS_NAMESPACE_LEN) {return NULL;}
    ANManager *pm = &g_manager;

    pthread_rwlock_wrlock(&pm->ns_lock);
    ANNameSpace * tmp = pm->head;
    while (tmp) {
	if (strcmp(tmp->name, name_space) == 0) { pthread_rwlock_unlock(&pm->ns_lock); return tmp; }
	tmp = tmp->next;
    }
    char ns_path[ADFS_MAX_PATH] = {0};
    snprintf(ns_path, sizeof(ns_path), "%s/%s", pm->path, name_space);
    int max_id = m_scan_kch(ns_path);
    char db_args[ADFS_MAX_PATH] = {0};
    snprintf(db_args, sizeof(db_args), "#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", pm->kc_apow, pm->kc_fbp, pm->kc_bnum *40, pm->kc_msiz);

    ANNameSpace * pns = malloc(sizeof(ANNameSpace));
    if (pns == NULL) { pthread_rwlock_unlock(&pm->ns_lock); return NULL; }
    anns_init(pns, name_space);

    char indexdb_path[ADFS_MAX_PATH] = {0};
    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s/index.kch%s", pm->path, name_space, db_args);
    pns->index_db = kcdbnew();
    int32_t res = kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK);
    if (!res) {goto err1;}

    for (int i=0; i <= max_id; ++i) {
	if (i < max_id) {
	    if (pns->create(pns, pm->path, db_args, S_READ_ONLY) == ADFS_ERROR) {goto err1;}
	}
	else {
	    if (pns->create(pns, pm->path, db_args, S_READ_WRITE) == ADFS_ERROR) {goto err1;}
	}
    }

    pns->prev = pm->tail;
    pns->next = NULL;
    if (pm->tail) {pm->tail->next = pns;}
    else {pm->head = pns;}
    pm->tail = pns;
    pthread_rwlock_unlock(&pm->ns_lock);
    return pns;
err1:
    free(pns);
    pthread_rwlock_unlock(&pm->ns_lock);
    return NULL;
}

// private
static int m_scan_kch(const char * dir)
{
    int max_id=0;
    DIR* dirp;
    struct dirent* direntp;

    dirp = opendir( dir );
    if( dirp != NULL ) {
	while ((direntp = readdir(dirp)) != NULL) {
	    int id = m_get_fileid(direntp->d_name);
	    max_id = id > max_id ? id : max_id;
	}
	closedir( dirp );
    }
    else {mkdir(dir, 0744);}
    return max_id;
}

// private
static int m_get_fileid(char * name)
{
    const int max_len = 8;
    if (name == NULL) {return -1;}
    if (strlen(name) > max_len) {return -1;}
    char *pos = strstr(name, ".kch");
    if (pos == NULL) {return -1;}
    if (strcmp(pos, ".kch") != 0) {return -1;}

    for (int i=0; i<pos-name; ++i) {
	if (name[i] < '0' || name[i] > '9') {return -1;}
    }
    char tmp[max_len];
    strncpy(tmp, name, pos-name);
    return atoi(tmp);
}

// private
static ADFS_RESULT m_init_log(const char *conf_file)
{
    char value[ADFS_FILENAME_LEN] = {0};
    // log_level
    if (conf_read(conf_file, "log_level", value, sizeof(value)) == ADFS_ERROR) { 
	log_out("manager", "[log_level]->config file error", LOG_LEVEL_SYSTEM); return ADFS_ERROR; 
    }
    g_log_level = atoi(value);
    if (g_log_level < 1 || g_log_level > 5) { 
	log_out("manager", "[log_level]->config value error", LOG_LEVEL_SYSTEM); return ADFS_ERROR; 
    }
    if (conf_read(conf_file, "log_file", value, sizeof(value)) == ADFS_ERROR) { 
	log_out("manager", "[log_file]->config file error", LOG_LEVEL_SYSTEM); return ADFS_ERROR; 
    }
    if (log_init(value) != 0) { 
	log_out("manager", "[log_file]->config value error", LOG_LEVEL_SYSTEM); return ADFS_ERROR; 
    }

    return ADFS_OK;
}

