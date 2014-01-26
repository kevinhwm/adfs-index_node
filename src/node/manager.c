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

static int m_init_log(cJSON *json);				// initialize
static int m_init_ns();						// initialize

static CNNameSpace * m_create_ns(const char *name_space);
static CNNameSpace * m_get_ns(const char * name_space);

CNManager g_manager;

int GNm_init(const char *conf_file, unsigned long mem_size) 
{
    CNManager *pm = &g_manager;
    memset(pm, 0, sizeof(CNManager));

    char *f_flag = _DFS_RUNNING_FLAG;
    if (access(f_flag, F_OK) != -1) {
	fprintf(stdout, "-> Another instance is running...\n-> Exit.\n");
	pm->another_running = 1;
	return -1;
    }
    else {
	pm->another_running = 0;
	FILE *f = fopen(f_flag, "w+");
	if (f == NULL) { return -1; }
	fclose(f);
    }

    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 100000;
    pm->kc_msiz = mem_size *1024*1024;
    pthread_rwlock_init(&pm->ns_lock, NULL);

    if (GNu_run() < 0) { return -1; }
    
    cJSON *json = conf_parse(conf_file);
    if (json == NULL) { return -1; }
    if (m_init_log(json) < 0) { conf_release(json); return -1; }
    conf_release(json);

    if (m_init_ns() < 0) { return -1; }
    return 0;
}

int GNm_exit() 
{
    CNManager * pm = &g_manager;
    if (pm->another_running) { return 0; }

    pthread_rwlock_wrlock(&pm->ns_lock);
    CNNameSpace * pns = pm->head;
    while (pns) {
	CNNameSpace *tmp = pns;
	pns = pns->next;
	tmp->release(tmp);
	free(tmp);
    }
    log_release();
    remove(_DFS_RUNNING_FLAG);
    pthread_rwlock_unlock(&pm->ns_lock);
    pthread_rwlock_destroy(&pm->ns_lock);
    return 0;
}

int GNm_save(const char * ns, const char *fname, size_t fname_len, void * fdata, size_t fdata_len)
{
    CNManager *pm = &g_manager;
    CNNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL && (pns = m_create_ns(name_space)) == NULL) {return -1;}
    if (pns->needto_split(pns)) {
	char db_args[_DFS_MAX_LEN] = {0};
	snprintf(db_args, sizeof(db_args), "#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
	if (pns->split_db(pns, MNGR_DATA_DIR, db_args) < 0) {return -1;}
    }
    if (!kcdbset(pns->tail->db, fname, fname_len, fdata, fdata_len)) {return -1;}
    pns->count_add(pns);
    char buf[16] = {0};
    sprintf(buf, "%d", pns->tail->id);
    if (!kcdbset(pns->index_db, fname, fname_len, buf, strlen(buf))) {return -1;}
    return 0;
}

int GNm_get(const char *ns, const char *fname, void ** ppfile_data, size_t *pfile_size)
{
    *ppfile_data = NULL;
    *pfile_size = 0;
    CNNameSpace * pns = NULL;
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

int GNm_erase(const char *ns, const char *fname)
{
    CNNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    pns = m_get_ns(name_space);
    if (pns == NULL) {return 0;}
    if ( kcdbremove(pns->index_db, fname, strlen(fname)) ) {return 0;}
    else {return -1;}
}

static CNNameSpace * m_get_ns(const char * name_space)
{
    CNManager *pm = &g_manager;
    pthread_rwlock_rdlock(&pm->ns_lock);
    CNNameSpace * tmp = g_manager.head;
    for (; tmp; tmp=tmp->next) {
	if (strcmp(tmp->name, name_space) == 0) { 
	    pthread_rwlock_unlock(&pm->ns_lock); 
	    return tmp; 
	}
    }
    pthread_rwlock_unlock(&pm->ns_lock);
    return NULL;
}

static CNNameSpace * m_create_ns(const char *name_space)
{
    if (strlen(name_space) >= _DFS_NAMESPACE_LEN) { return NULL; }
    CNManager *pm = &g_manager;

    pthread_rwlock_wrlock(&pm->ns_lock);
    CNNameSpace * tmp = pm->head;
    for (; tmp; tmp=tmp->next) {
	if (strcmp(tmp->name, name_space) == 0) { 
	    pthread_rwlock_unlock(&pm->ns_lock); 
	    return tmp; 
	}
    }

    char db_args[_DFS_MAX_LEN] = {0};
    snprintf(db_args, sizeof(db_args), "#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", pm->kc_apow, pm->kc_fbp, pm->kc_bnum*400, pm->kc_msiz);

    CNNameSpace * pns = malloc(sizeof(CNNameSpace));
    if (pns == NULL) { goto err1; }
    if (GNns_init(pns, name_space, MNGR_DATA_DIR, db_args) < 0) { goto err2; }

    pns->prev = pm->tail;
    pns->next = NULL;
    if (pm->tail) { pm->tail->next = pns; }
    else {pm->head = pns;}
    pm->tail = pns;
    pthread_rwlock_unlock(&pm->ns_lock);

    return pns;
err2:
    pns->release(pns);
    free(pns);
err1:
    pthread_rwlock_unlock(&pm->ns_lock);
    return NULL;
}

static int m_init_log(cJSON *json)
{
    cJSON *j_tmp = NULL;
    j_tmp = cJSON_GetObjectItem(json, "log_level");
    if (j_tmp == NULL) {
	fprintf(stderr, "Config file error. No 'log_level' section.\n-> Exit.\n");
        return -1;
    }
    LOG_LEVEL log_level = j_tmp->valueint;
    if (log_init(log_level, NULL) < 0) {
	fprintf(stderr, "[log_level]-> Config value error.\n-> Exit.\n");
	return -1;
    }
    return 0;
}

static int m_init_ns()
{
    DIR *dp = NULL;
    struct dirent *dirp;
    dp = opendir(MNGR_DATA_DIR);
    while ((dirp = readdir(dp)) != NULL) {
	if ((dirp->d_type == DT_DIR) && (dirp->d_name[0] != '.')) {
	    if (m_create_ns(dirp->d_name) == NULL) { 
		closedir(dp); 
		return -1;
	    }
	}
    }
    closedir(dp);
    return 0;
}

