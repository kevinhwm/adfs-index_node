/* manager.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kclangc.h>

#include "namespace.h"
#include "manager.h"
#include "../adfs.h"

static ANNameSpace * m_create_ns(const char *name_space);
static ANNameSpace * m_get_ns(const char * name_space);
static int m_init_log(cJSON *json);
static int m_scan_kch(const char * dir);
static int m_get_fileid(char * name);

ANManager g_manager;

int anm_init(const char * conf_file, unsigned long mem_size) 
{
    ANManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));

    char *f_flag = ADFS_RUNNING_FLAG;
    if (access(f_flag, F_OK) != -1) {
	fprintf(stdout, "-> another instance is running...\n-> exit.\n");
	pm->another_running = 1;
	return -1;
    }
    else {
	pm->another_running = 0;
	time_t t;
	char stime[64] = {0};
	time(&t);
	FILE *f = fopen(f_flag, "wb+");
	if (f == NULL) { return -1; }
	fprintf(f, "%s", ctime_r(&t, stime));
	fclose(f);
    }

    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 100000;
    pm->kc_msiz = mem_size *1024*1024;
    pthread_rwlock_init(&pm->ns_lock, NULL);
    strncpy(pm->data_dir, "data", sizeof(pm->data_dir));
    strncpy(pm->log_dir, "log", sizeof(pm->log_dir));
    sprintf(pm->core_log, "%s/ancore.log", pm->log_dir);

    if (anu_init() < 0) { return -1; }
    
    cJSON *json = conf_parse(conf_file);
    if (json == NULL) { return -1; }
    if (m_init_log(json) < 0) { conf_release(json); return -1; }
    conf_release(json);

    DIR *dp = NULL;
    struct dirent *dirp;
    dp = opendir(pm->data_dir);
    while ((dirp = readdir(dp)) != NULL) {
	if ((dirp->d_type == DT_DIR) && (dirp->d_name[0] != '.')) {m_create_ns(dirp->d_name);}
    }
    closedir(dp);

    return 0;
}

int anm_exit() 
{
    ANManager * pm = &g_manager;
    if (pm->another_running) { return 0; }

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
    remove(ADFS_RUNNING_FLAG);
    return 0;
}

int anm_save(const char * ns, const char *fname, size_t fname_len, void * fdata, size_t fdata_len)
{
    ANManager *pm = &g_manager;
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL && (pns = m_create_ns(name_space)) == NULL) {return -1;}
    if (pns->needto_split(pns)) {
	char db_args[ADFS_MAX_LEN] = {0};
	snprintf(db_args, sizeof(db_args), "#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
	if (pns->split_db(pns, pm->data_dir, db_args) < 0) {return -1;}
    }
    if (!kcdbset(pns->tail->db, fname, fname_len, fdata, fdata_len)) {return -1;}
    pns->count_add(pns);
    char buf[16] = {0};
    sprintf(buf, "%d", pns->tail->id);
    if (!kcdbset(pns->index_db, fname, fname_len, buf, strlen(buf))) {return -1;}
    return 0;
}

int anm_get(const char *ns, const char *fname, void ** ppfile_data, size_t *pfile_size)
{
    *ppfile_data = NULL;
    *pfile_size = 0;
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL) {return -1;}

    size_t len = 0;
    char *id = kcdbget(pns->index_db, fname, strlen(fname), &len);
    if (id == NULL) {return -1;}
    NodeDB *pn = pns->get(pns, atoi(id));
    if (pn == NULL) {kcfree(id); return -1;}
    *ppfile_data = kcdbget(pn->db, fname, strlen(fname), pfile_size);
    kcfree(id);
    return 0;
}

int anm_erase(const char *ns, const char *fname)
{
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL) {return 0;}
    if ( kcdbremove(pns->index_db, fname, strlen(fname)) ) {return 0;}
    else {return -1;}
}

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
    char ns_path[ADFS_MAX_LEN] = {0};
    snprintf(ns_path, sizeof(ns_path), "%s/%s", pm->data_dir, name_space);
    int max_id = m_scan_kch(ns_path);
    char db_args[ADFS_MAX_LEN] = {0};
    snprintf(db_args, sizeof(db_args), "#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", pm->kc_apow, pm->kc_fbp, pm->kc_bnum *200, pm->kc_msiz);

    ANNameSpace * pns = malloc(sizeof(ANNameSpace));
    if (pns == NULL) { pthread_rwlock_unlock(&pm->ns_lock); return NULL; }
    anns_init(pns, name_space);

    char indexdb_path[ADFS_MAX_LEN] = {0};
    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s/index.kch%s", pm->data_dir, name_space, db_args);
    pns->index_db = kcdbnew();
    int32_t res = kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK);
    if (!res) {goto err1;}

    for (int i=0; i <= max_id; ++i) {
	if (i < max_id) {
	    if (pns->create(pns, pm->data_dir, db_args, S_READ_ONLY) < 0) {goto err1;}
	}
	else {
	    if (pns->create(pns, pm->data_dir, db_args, S_READ_WRITE) < 0) {goto err1;}
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

static int m_scan_kch(const char * dir)
{
    int max_id=0;
    DIR* dp;
    struct dirent* dirp;

    dp = opendir( dir );
    if( dp != NULL ) {
	while ((dirp = readdir(dp)) != NULL) {
	    int id = m_get_fileid(dirp->d_name);
	    max_id = id > max_id ? id : max_id;
	}
	closedir( dp );
    }
    else {mkdir(dir, 0744);}
    return max_id;
}

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
    memset(tmp, 0, max_len);
    strncpy(tmp, name, pos-name);
    return atoi(tmp);
}

static int m_init_log(cJSON *json)
{
    // log_level
    cJSON *j_tmp = NULL;
    j_tmp = cJSON_GetObjectItem(json, "log_level");
    if (j_tmp == NULL) {
	fprintf(stderr, "[log_level]->config file error");
        return -1;
    }
    LOG_LEVEL log_level = j_tmp->valueint;
    if (log_init(log_level) < 0) {
	fprintf(stderr, "[log_level]->config value error");
	return -1;
    }
    return 0;
}

