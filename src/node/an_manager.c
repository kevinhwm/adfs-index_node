/* Antiy Labs. Basic Platform R & D Center.
 * an_manager.c
 *
 * huangtao@antiy.com
 */

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kclangc.h>

#include "adfs.h"
#include "an_namespace.h"
#include "an_manager.h"

static ANNameSpace * m_create_ns(const char *name_space);
static ADFS_RESULT split_db(ANNameSpace * pns);
static int m_count_kch(const char * dir);
static ADFS_RESULT m_identify_kch(char * name);
static ANNameSpace * m_get_ns(const char * name_space);
static ADFS_RESULT m_init_log(const char *conf_file);

ANManager g_manager;
LOG_LEVEL g_log_level = LOG_LEVEL_DEBUG;

ADFS_RESULT anm_init(const char * conf_file, const char *path, unsigned long mem_size) 
{
    char msg[1024] = {0};
    ANManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));
    strncpy(pm->path, path, sizeof(pm->path));
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;

    DIR *dirp;
    struct dirent *ent;
    if( (dirp = opendir(path)) == NULL ) 
	return ADFS_ERROR;

    while ((ent = readdir(dirp)) != NULL) {
	if (ent->d_type == 4 && ent->d_name[0] != '.') {
	    DBG_PRINTSN(ent->d_name);
	    m_create_ns(ent->d_name);
	}
    }
    closedir(dirp);
    char f_flag[1024] = {0};
    snprintf(f_flag, sizeof(f_flag), "%s/adfs.flag", path);
    if (access(f_flag, F_OK) != -1) {
	snprintf(msg, sizeof(msg), "[%s]->there is another instance is running.", f_flag);
	log_out("manager", msg, LOG_LEVEL_FATAL);
	return ADFS_ERROR;
    }
    else {
	time_t t = time(NULL);
	struct tm *lt = localtime(&t);

	FILE *f = fopen(f_flag, "wb+");
	fprintf(f, "%s", asctime(lt));
	fclose(f);
    }
    snprintf(msg, sizeof(msg), "[%s]->init db path", path);
    log_out("manager", msg, LOG_LEVEL_INFO);

    // init log
    if (m_init_log(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;

    return ADFS_OK;
}

void anm_exit() 
{
    ANManager * pm = &g_manager;
    ANNameSpace * pns = pm->head;
    while (pns) {
	ANNameSpace *tmp = pns;
	pns = pns->next;
	kcdbclose(tmp->index_db);
	kcdbdel(tmp->index_db);
	tmp->release_all(tmp);
	free(tmp);
    }
    log_release();
    remove("./adfs.flag");
}

ADFS_RESULT anm_save(const char * ns, const char *fname, size_t fname_len, void * fdata, size_t fdata_len)
{
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL)
	name_space = "default";
    pns = m_get_ns(name_space);
    if (pns == NULL)
	pns = m_create_ns(name_space);
    if (pns == NULL)
	return ADFS_ERROR;

    // check number and split db
    NodeDB * node = pns->tail;
    if (node->number >= NODE_MAX_FILE_NUM) {
	if (split_db(pns) == ADFS_ERROR)
	    return ADFS_ERROR;
	node = pns->tail;
    }

    // save into db
    if (!kcdbset(pns->tail->db, fname, fname_len, fdata, fdata_len))
	return ADFS_ERROR;
    // save into index
    char buf[16] = {0};
    sprintf(buf, "%d", pns->tail->id);
    if (!kcdbset(pns->index_db, fname, fname_len, buf, strlen(buf)))
	return ADFS_ERROR;

    return ADFS_OK;
}

void anm_get(const char *ns, const char *fname, void ** ppfile_data, size_t *pfile_size)
{
    *ppfile_data = NULL;
    *pfile_size = 0;
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL)
	name_space = "default";
    pns = m_get_ns(name_space);
    if (pns == NULL)
	return ;

    size_t len = 0;
    char *id = kcdbget(pns->index_db, fname, strlen(fname), &len);
    if (id == NULL)
	return ;
    NodeDB *pn = pns->get(pns, atoi(id));
    *ppfile_data = kcdbget(pn->db, fname, strlen(fname), pfile_size);
    kcfree(id);
}

ADFS_RESULT anm_remove(const char *ns, const char *fname)
{
    ANNameSpace * pns = NULL;
    const char *name_space = ns;
    if (name_space == NULL)
	name_space = "default";
    pns = m_get_ns(name_space);
    if (pns == NULL)
	return ADFS_ERROR;
    size_t len = 0;
    char *id = kcdbseize(pns->index_db, fname, strlen(fname), &len);
    if (id == NULL)
	return ADFS_ERROR;
    NodeDB *pn = pns->get(pns, atoi(id));
    kcdbremove(pn->db, fname, strlen(fname));
    kcfree(id);
    return ADFS_OK;
}

////////////////////////////////////////////////////////////////////////////////
//private
static ANNameSpace * m_create_ns(const char *name_space)
{
    ANManager *pm = &g_manager;
    if (strlen(name_space) >= ADFS_NAMESPACE_LEN)
	return NULL;

    char ns_path[ADFS_MAX_PATH] = {0};
    snprintf(ns_path, sizeof(ns_path), "%s/%s", pm->path, name_space);
    int node_num = m_count_kch(ns_path);
    // index db of ADFS-Node
    char indexdb_path[ADFS_MAX_PATH] = {0};
    if (snprintf(indexdb_path, sizeof(indexdb_path), "%s/index.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
		ns_path, pm->kc_apow, pm->kc_fbp, pm->kc_bnum *40, pm->kc_msiz) >= sizeof(indexdb_path))
    {
	return NULL;
    }

    ANNameSpace * pns = malloc(sizeof(ANNameSpace));
    anns_init(pns, name_space);
    pns->index_db = kcdbnew();
    int32_t res = kcdbopen( pns->index_db, indexdb_path, KCOWRITER|KCOCREATE );
    if ( !res ) {
	free(pns);
	return NULL;
    }

    // node db of ADFS-Node
    char tmp_path[ADFS_MAX_PATH] = {0};
    for (int i=1; i <= node_num; i++)
    {
	memset(tmp_path, 0, sizeof(tmp_path));
	if (snprintf(tmp_path, sizeof(tmp_path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
		    ns_path, i, pm->kc_apow, pm->kc_fbp, pm->kc_bnum*3, pm->kc_msiz) >= sizeof(tmp_path))
	{
	    return NULL;
	}

	if (i < node_num) {
	    DBG_PRINTS("create ro db file\n");
	    if (pns->create(pns, i, tmp_path, strlen(tmp_path), S_READ_ONLY) == ADFS_ERROR)
		return NULL;
	}
	else {
	    DBG_PRINTS("create rw db file\n");
	    if (pns->create(pns, i, tmp_path, strlen(tmp_path), S_READ_WRITE) == ADFS_ERROR)
		return NULL;
	}
    }
    pns->pre = pm->tail;
    pns->next = NULL;
    if (pm->tail)
	pm->tail->next = pns;
    else
	pm->head = pns;
    pm->tail = pns;
    return pns;
}

// private
static ADFS_RESULT split_db(ANNameSpace * pns)
{
    ANManager * pm = &g_manager;
    NodeDB * last_node = pns->tail;

    if (pns->switch_state(pns, last_node->id, S_READ_ONLY) == ADFS_ERROR)
	return ADFS_ERROR;

    char path[ADFS_MAX_PATH] = {0};
    if (snprintf(path, sizeof(path), "%s/%lu.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
		pm->path, pns->number, pm->kc_apow, pm->kc_fbp, pm->kc_bnum*3, pm->kc_msiz) >= sizeof(path))
    {
	return ADFS_ERROR;
    }

    return pns->create(pns, pns->number, path, strlen(path), S_READ_WRITE);
}

// private
static int m_count_kch(const char * dir)
{
    int count=0;
    DIR* dirp;
    struct dirent* direntp;

    dirp = opendir( dir );
    if( dirp != NULL ) {
	while ((direntp = readdir(dirp)) != NULL) {
	    if (m_identify_kch(direntp->d_name) == ADFS_OK)
		count++;
	}
	closedir( dirp );

	if (count>0)
	    return count;
	else
	    return 1;
    }
    else {
	mkdir(dir, 0755);
	return 1;
    }
}

// private
static ADFS_RESULT m_identify_kch(char * name)
{
    if (name == NULL)
	return ADFS_ERROR;
    if (strlen(name) > 8)
	return ADFS_ERROR;
    char *pos = strstr(name, ".kch");
    if (pos == NULL)
	return ADFS_ERROR;

    for (int i=0; i < pos-name; i++) {
	if (name[i] < '0' || name[i] > '9')
	    return ADFS_ERROR;
    }

    if (strcmp(pos, ".kch") != 0)
	return ADFS_ERROR;
    return ADFS_OK;
}

// private
static ANNameSpace * m_get_ns(const char * name_space)
{
    ANNameSpace * tmp = g_manager.head;
    while (tmp) {
	if (strcmp(tmp->name, name_space) == 0)
	    return tmp;
	tmp = tmp->next;
    }
    return NULL;
}

static ADFS_RESULT m_init_log(const char *conf_file)
{
    char value[ADFS_FILENAME_LEN] = {0};
    // log_level
    if (conf_read(conf_file, "log_level", value, sizeof(value)) == ADFS_ERROR) {
	log_out("manager", "[log_level]->config file error", LOG_LEVEL_SYSTEM);
        return ADFS_ERROR;
    }
    g_log_level = atoi(value);
    if (g_log_level < 1 || g_log_level > 5) {
	log_out("manager", "[log_level]->config value error", LOG_LEVEL_SYSTEM);
	return ADFS_ERROR;
    }
    if (conf_read(conf_file, "log_file", value, sizeof(value)) == ADFS_ERROR) {
	log_out("manager", "[log_file]->config file error", LOG_LEVEL_SYSTEM);
	return ADFS_ERROR;
    }
    if (log_init(value) != 0) {
	log_out("manager", "[log_file]->config value error", LOG_LEVEL_SYSTEM);
	return ADFS_ERROR;
    }

    return ADFS_OK;
}

