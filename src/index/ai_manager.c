/* ai_manager.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <string.h>
#include <time.h>
#include <dirent.h>     // opendir
#include <unistd.h>
#include <sys/stat.h>   // chmod
#include <kclangc.h>
#include <curl/curl.h>

#include "adfs.h"
#include "ai_manager.h"
#include "ai_record.h"

static ADFS_RESULT m_init_log(const char *conf_file);
static ADFS_RESULT m_init_stat(const char *conf_file);
static ADFS_RESULT m_init_ns(const char *conf_file);
static ADFS_RESULT m_init_zone(const char *conf_file);
static AIZone * m_create_zone(const char *name, int weight);
static ADFS_RESULT m_create_ns(const char *name);
static AINameSpace * m_get_ns(const char *ns);
static AINode * m_get_node(const char *node, size_t len);
static AIZone * m_choose_zone(const char * record);
static char * m_get_history(const char *, int);

static ADFS_RESULT m_erase();
static ADFS_RESULT e_traverse(AINameSpace *pns);
static const char * e_parse(const char *ns, const char *fname, const char *record);
static ADFS_RESULT e_connect(const char *ns, const char *fname, const char *record, int len);

AIManager g_manager;
unsigned long g_MaxFileSize;
LOG_LEVEL g_log_level = LOG_LEVEL_DEBUG;
int g_erase_mode = 0;
int g_another_running = 0;

ADFS_RESULT aim_init(const char *conf_file, const char *path, unsigned long mem_size, unsigned long max_file_size)
{
    srand(time(NULL));

    char msg[1024] = {0};
    AIManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));

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
	snprintf(msg, sizeof(msg), "[%s]->there is another instance is running.", f_flag);
	log_out("manager", msg, LOG_LEVEL_FATAL);
	g_another_running = 1;
	return ADFS_ERROR;
    }
    else {
	time_t t = time(NULL);
	struct tm * lt = localtime(&t);
	FILE *f = fopen(f_flag, "wb+");
	fprintf(f, "%s", asctime(lt));
	fclose(f);
    }
    snprintf(msg, sizeof(msg), "[%s]->init db path", path);
    log_out("manager", msg, LOG_LEVEL_INFO);

    strncpy(pm->path, path, sizeof(pm->path));
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;
    DBG_PRINTSN("50");
    // init log
    if (m_init_log(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;
    DBG_PRINTSN("60");
    // init statistics
    if (m_init_stat(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;
    DBG_PRINTSN("70");
    // init namespace
    if (m_init_ns(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;
    DBG_PRINTSN("80");
    // init zone
    if (m_init_zone(conf_file) == ADFS_ERROR)
        return ADFS_ERROR;
    g_MaxFileSize = max_file_size *1024*1024;
    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);

    DBG_PRINTSN("100");
    // work mode 
    char value[ADFS_FILENAME_LEN] = {0};
    if (conf_read(conf_file, "work_mode", value, sizeof(value)) == ADFS_ERROR) {
	log_out("manager", "[work_mode]->config file error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    snprintf(msg, sizeof(msg), "[%s]->work mode", value);
    log_out("manager", msg, LOG_LEVEL_INFO);
    if (strcmp(value, "erase") == 0) {
	printf("\nIndex will work in 'erase mode'! \n"
		"When it is done, you should restart it manually. \n");
	while (1) {
	    printf("\nAre you sure ? (y/n) ");
	    int i = getchar();
	    if (i == 'y') {
		g_erase_mode = 1;
		log_out("manager", "work in 'erase' mode", LOG_LEVEL_INFO);
		printf("\nwork in 'erase' mode.\n");
		if (m_erase() == ADFS_OK)
		    log_out("manager", "erase ok.", LOG_LEVEL_INFO);
		else
		    log_out("manager", "erase failed.", LOG_LEVEL_INFO);
		return ADFS_OK;
	    }
	    else if (i == 'n') {
		log_out("manager", "user quit", LOG_LEVEL_INFO);
		return ADFS_ERROR;
	    }
	}
    }
    else if (strcmp(value, "normal") == 0) {
	printf("\nwork in 'normal' node.\n");
	return ADFS_OK;
    }
    else
	log_out("manager", "[work_mode]->config value error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
}

void aim_exit()
{
    AIManager *pm = &g_manager;
    AIZone *pz = pm->z_head;
    while (pz) {
        AIZone *tmp = pz;
        pz = pz->next;
        tmp->release_all(tmp);
        free(tmp);
    }

    AINameSpace *pns = pm->ns_head;
    while (pns) {
        AINameSpace *tmp = pns;
        pns = pns->next;
	kcdbclose(tmp->index_db);
	kcdbdel(tmp->index_db);
        free(tmp);
    }

    if (pm->s_upload.release)
	pm->s_upload.release(&(pm->s_upload));
    if (pm->s_download.release)
	pm->s_download.release(&(pm->s_download));
    if (pm->s_delete.release)
	pm->s_delete.release(&(pm->s_delete));

    log_release();
    curl_global_cleanup();

    if (!g_another_running) {
	char f_flag[1024] = {0};
	snprintf(f_flag, sizeof(f_flag), "%s/adfs.flag", pm->path);
	remove(f_flag);
    }
}

ADFS_RESULT aim_upload(const char *ns, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    AIManager *pm = &g_manager;

    int exist = 1;
    char *old_list = NULL;
    size_t old_list_len;
    const char *name_space = ns;
    if (name_space == NULL)
	name_space = "default";

    AINameSpace *pns = NULL;
    pns = m_get_ns(name_space);
    if (pns == NULL) 
	return ADFS_ERROR;
    old_list = kcdbget(pns->index_db, fname, strlen(fname), &old_list_len);
    if (old_list == NULL || old_list[old_list_len-1] == '$')
        exist = 0;
    if (exist && !overwrite)		// exist and not overwrite
	goto ok1;

    // send file
    AIPosition *pp;	// may be used in "rollback" tag
    AIRecord air;
    air_init(&air);

    AIZone *pz = pm->z_head;
    while (pz) {
	AINode * pn = pz->rand_choose(pz);
	char url[ADFS_MAX_PATH] = {0};
	snprintf(url, sizeof(url), "http://%s/upload_file/%s%.*s?namespace=%s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid, name_space);

	if (aic_upload(pn, url, fname, fdata, fdata_len) == ADFS_ERROR) {
	    printf("upload error: %s\n", url);
	    goto rollback;
	}
	air.add(&air, pz->name, pn->ip_port);
	pz = pz->next;
    }

    // add record
    char *record = air.get_string(&air);
    if (record == NULL)
	goto err1;

    DBG_PRINTSN(record);
    if (old_list == NULL) {
	kcdbset(pns->index_db, fname, strlen(fname), record, strlen(record));
    }
    else {
	long len = strlen(record) + 2;
	char *new_list = malloc(len);
	if (new_list == NULL) 
	    goto err2;
	if (exist)
	    snprintf(new_list, len, "$%s", record);
	else
	    snprintf(new_list, len, "%s", record);
	// kcdbset(pns->index_db, fname, strlen(fname), new_list, strlen(new_list));
	kcdbappend(pns->index_db, fname, strlen(fname), new_list, strlen(new_list));
	free(new_list);
    }
    pm->s_upload.inc(&(pm->s_upload));

    free(record);
    air.release(&air);
ok1:
    kcfree(old_list);
    return ADFS_OK;
rollback:
    pp = air.head;
    while (pp) {
	char *pos_sharp = strstr(pp->zone_node, "#");
	AINode *pn = m_get_node(pos_sharp + 1, strlen(pos_sharp +1));
	if (pn != NULL) {
	    char url[ADFS_MAX_PATH] = {0};
	    if (name_space)
		snprintf(url, sizeof(url), "http://%s/erase/%s%.*s?namespace=%s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid, name_space);
	    else
		snprintf(url, sizeof(url), "http://%s/erase/%s%.*s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid);
	    aic_erase(pn, url);		// do not care about success or failure.
	}
	pp = pp->next;
    }
    goto err1;
err2:
    free(record);
err1:
    air.release(&air);
    kcfree(old_list);
    return ADFS_ERROR;
}

// return value:
// NULL:    file not found
// url :    "char *" must be freed by caller
char * aim_download(const char *ns, const char *fname, const char *history)
{
    AIManager *pm = &g_manager;
    const char *name_space = ns;
    if (name_space == NULL)
	name_space = "default";
    AINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) 
	return NULL;

    int order = 0;
    if (history != NULL) {
	for (int i=0; i<strlen(history); ++i)
	    if (history[i] < '0' || history[i] > '9')
		return NULL;
	if ((order = atoi(history)) < 0)
	    return NULL;
    }

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) 
	return NULL;
    char *url = (char *)malloc(ADFS_MAX_PATH);
    if (url == NULL) 
	goto err1;
    memset(url, 0, ADFS_MAX_PATH);

    char *record = m_get_history(line, order);
    if (record == NULL)
	goto err2;

    AIZone *pz = m_choose_zone(record + ADFS_UUID_LEN);
    if (pz == NULL)
	goto err3;

    char *pos_zone = strstr(record, pz->name);
    char *pos_sharp = strstr(pos_zone, "#");
    char *pos_split = strstr(pos_sharp, "|");
    if (pos_split)
	snprintf(url, ADFS_MAX_PATH, "http://%.*s/download/%s%.*s", (int)(pos_split-pos_sharp-1), pos_sharp+1, fname, ADFS_UUID_LEN, record);
    else
	snprintf(url, ADFS_MAX_PATH, "http://%s/download/%s%.*s", pos_sharp+1, fname, ADFS_UUID_LEN, record);
    strncat(url, "?namespace=", ADFS_MAX_PATH);
    strncat(url, pns->name, ADFS_MAX_PATH);
    pm->s_download.inc(&(pm->s_download));

    free(record);
    kcfree(line);
    return url;
err3:
    free(record);
err2:
    free(url);
err1:
    kcfree(line);
    return NULL;
}

ADFS_RESULT aim_delete(const char *ns, const char *fname)
{
    AIManager *pm = &g_manager;
    const char *name_space = ns;
    if (name_space == NULL)
	name_space = "default";
    AINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) 
	return ADFS_ERROR;

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) 
	return ADFS_ERROR;

    if (line[vsize-1] == '$')
	return ADFS_OK;

    char *new_line = malloc(vsize+4);
    if (new_line == NULL)
	goto err1;
    memset(new_line, 0, vsize+4);
    strcpy(new_line, line);
    strcat(new_line, "$");
    kcdbset(pns->index_db, fname, strlen(fname), new_line, strlen(new_line));
    pm->s_delete.inc(&(pm->s_delete));
    free(new_line);
    kcfree(line);
    return ADFS_OK;
err1:
    kcfree(line);
    return ADFS_ERROR;
}

char * aim_status()
{
    int size = 8192;
    char *p = malloc(size);
    if (p == NULL)
	return NULL;
    memset(p, 0, size);

    AIManager *pm = &g_manager;
    AIZone *pz = pm->z_head;
    while (pz) {
	strncat(p, "<li><font color=\"blue\">Zone: ", size);
	strncat(p, pz->name, size);
	strncat(p, "</font></li><table border=\"1\">\n", size);
	strncat(p, "<tr><th>node</th><th>status</th></tr>\n", size);
	AINode *pn = pz->head;
	while (pn) {
	    strncat(p, "<tr><td>", size);
	    strncat(p, pn->ip_port, size);
	    strncat(p, "</td><td ", size);
	    char url[1024] = {0};
	    snprintf(url, sizeof(url), "http://%s/status", pn->ip_port);
	    if (aic_status(pn, url) == ADFS_OK) 
		strncat(p, "bgcolor=\"green\"><font color=\"white\">alive</font>", size);
	    else 
		strncat(p, "bgcolor=\"red\"><font color=\"white\">lost</font>", size);
	    strncat(p, "</td></tr>", size);
	    pn = pn->next;
	}
	strncat(p, "</table>", size);
    	pz = pz->next;
    }

    time_t t_cur = time(NULL);
    struct tm *lt = localtime(&t_cur);
    char str_time[32] = {0};
    strftime(str_time, sizeof(str_time), "current time: %H:%M", lt);
    strncat(p, "<p>", size);
    strncat(p, str_time, size);
    strncat(p, "</p>", size);
    int *pcount = pm->s_upload.get(&(pm->s_upload), &t_cur);
    for (int i=0; i<pm->s_upload.scope; ++i) {
	if (pcount < pm->s_upload.count)
	    pcount += pm->s_upload.scope;
	char tmp[128] = {0};
	snprintf(tmp, sizeof(tmp), "%d", *pcount);
	strncat(p, tmp, size);
	strncat(p, "|", size);
	pcount--;
    }

    return p;
}

// private
static ADFS_RESULT m_init_zone(const char *conf_file)
{
    char msg[1024];	// log msg
    char value[ADFS_FILENAME_LEN] = {0};
    // create zone
    if (conf_read(conf_file, "zone_num", value, sizeof(value)) == ADFS_ERROR) {
	log_out("manager", "[zone_num]->config file error", LOG_LEVEL_ERROR);
        return ADFS_ERROR;
    }
    int zone_num = atoi(value);
    if (zone_num <= 0) {
	log_out("manager", "[zone_num]->config value error", LOG_LEVEL_ERROR);
        return ADFS_ERROR;
    }
    for (int i=0; i<zone_num; ++i)
    {
        char key[ADFS_FILENAME_LEN] = {0};
        char name[ADFS_FILENAME_LEN] = {0};
        char weight[ADFS_FILENAME_LEN] = {0};
        char num[ADFS_FILENAME_LEN] = {0};
        snprintf(key, sizeof(key), "zone%d_name", i);
        if (conf_read(conf_file, key, name, sizeof(name)) == ADFS_ERROR) {
	    snprintf(msg, sizeof(msg), "[%s]->config file error", key);
	    log_out("manager", msg, LOG_LEVEL_ERROR);
            return ADFS_ERROR;
	}
        snprintf(key, sizeof(key), "zone%d_weight", i);
        if (conf_read(conf_file, key, weight, sizeof(weight)) == ADFS_ERROR) {
	    snprintf(msg, sizeof(msg), "[%s]->config file error", key);
	    log_out("manager", msg, LOG_LEVEL_ERROR);
            return ADFS_ERROR;
	}
	AIZone *pz = m_create_zone(name, atoi(weight));
	if (pz == NULL) {
	    snprintf(msg, sizeof(msg), "[malloc]->create zone error");
	    log_out("manager", msg, LOG_LEVEL_ERROR);
	    return ADFS_ERROR;
	}
        snprintf(key, sizeof(key), "zone%d_num", i);
        if (conf_read(conf_file, key, num, sizeof(num)) == ADFS_ERROR) {
	    snprintf(msg, sizeof(msg), "[%s]->config file error", key);
	    log_out("manager", msg, LOG_LEVEL_ERROR);
            return ADFS_ERROR;
	}
        int node_num = atoi(num);
        if (node_num <= 0) {
	    snprintf(msg, sizeof(msg), "[node_num]->config file error");
	    log_out("manager", msg, LOG_LEVEL_ERROR);
            return ADFS_ERROR;
	}
        for (int j=0; j<node_num; ++j) {
            snprintf(key, sizeof(key), "zone%d_%d", i, j);
	    if (conf_read(conf_file, key, value, sizeof(value)) == ADFS_ERROR) {
		snprintf(msg, sizeof(msg), "[%s]->config file error", key);
		log_out("manager", msg, LOG_LEVEL_ERROR);
		return ADFS_ERROR;
	    }
            if (strlen(value) <= 0) {
		snprintf(msg, sizeof(msg), "[%s]->config value error", key);
		log_out("manager", msg, LOG_LEVEL_ERROR);
                return ADFS_ERROR;
	    }
	    // create node
            if (pz->create(pz, value) == ADFS_ERROR) {
		snprintf(msg, sizeof(msg), "[malloc]->create node error");
		log_out("manager", msg, LOG_LEVEL_ERROR);
                return ADFS_ERROR;
	    }
        }
    }
    return ADFS_OK;
}

static ADFS_RESULT m_init_ns(const char *conf_file)
{
    char msg[1024];
    char value[ADFS_FILENAME_LEN] = {0};
    // create namespace
    if (conf_read(conf_file, "namespace_num", value, sizeof(value)) == ADFS_ERROR) {
	snprintf(msg, sizeof(msg), "[namespace_num]->config file error");
	log_out("manager", msg, LOG_LEVEL_ERROR);
        return ADFS_ERROR;
    }
    int ns_num = atoi(value);
    if (ns_num <= 0 || ns_num >100) {
	snprintf(msg, sizeof(msg), "[namespace_num]->config value error");
	log_out("manager", msg, LOG_LEVEL_ERROR);
        return ADFS_ERROR;
    }
    if (m_create_ns("default") == ADFS_ERROR) {
	snprintf(msg, sizeof(msg), "[default]->create namespace error");
	log_out("manager", msg, LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    for (int i=0; i<ns_num; ++i) {
        char key[ADFS_FILENAME_LEN] = {0};
	snprintf(key, sizeof(key), "namespace_%d", i);
	if (conf_read(conf_file, key, value, sizeof(value)) == ADFS_ERROR) {
	    snprintf(msg, sizeof(msg), "[%s]->config file error", key);
	    log_out("manager", msg, LOG_LEVEL_ERROR);
	    return ADFS_ERROR;
	}
	if (m_create_ns(value) == ADFS_ERROR) {
	    snprintf(msg, sizeof(msg), "[%s]->create namespace error", key);
	    log_out("manager", msg, LOG_LEVEL_ERROR);
	    return ADFS_ERROR;
	}
    }
    return ADFS_OK;
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

static ADFS_RESULT m_init_stat(const char *conf_file)
{
    AIManager *pm = &g_manager;
    char value[ADFS_FILENAME_LEN] = {0};
    // statistics
    if (conf_read(conf_file, "stat_hour", value, sizeof(value)) == ADFS_ERROR) {
	log_out("manager", "[stat_hour]->config file error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    int stat_hour = atoi(value);
    if (stat_hour < 1 || stat_hour > 24) {
	log_out("manager", "[stat_hour]->config value error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }

    unsigned long stat_start = (unsigned long)time(NULL);
    int stat_min = stat_hour * 60;
    if (stat_init(&(pm->s_upload), stat_start, stat_min) == ADFS_ERROR) {
	log_out("manager", "[malloc]->stat init error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    if (stat_init(&(pm->s_download), stat_start, stat_min) == ADFS_ERROR) {
	log_out("manager", "[malloc]->stat init error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    if (stat_init(&(pm->s_delete), stat_start, stat_min) == ADFS_ERROR) {
	log_out("manager", "[malloc]->stat init error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    return ADFS_OK;
}

// private
static AIZone * m_choose_zone(const char *record)
{
    if (record == NULL)
	return NULL;
    AIManager *pm = &g_manager;
    AIZone *biggest_z = NULL;   // (weight/count)
    AIZone *least_z = NULL;

    AIZone *pz = pm->z_head;
    for (; pz; pz = pz->next)
    {
        if (strstr(record, pz->name) == NULL)
            continue;

        if (pz->count == 0) {
            pz->count += 1;
            return pz;
        }
        else if (biggest_z == NULL)
            biggest_z = pz;
        else {
            biggest_z = (pz->weight / pz->count) > (biggest_z->weight / biggest_z->count) ? pz : biggest_z;

            if (least_z == NULL)
                least_z = pz;
            else
                least_z = (pz->weight / pz->count) < (least_z->weight / least_z->count) ? pz : least_z;
        }
    }
    if (biggest_z) {
	biggest_z->count += 1;
	if (biggest_z == least_z) {
	    pz = pm->z_head;
	    while (pz) {
		pz->count = 0;
		pz = pz->next;
	    }
	}
    }
    return biggest_z;
}

// private
static AINameSpace * m_get_ns(const char *ns)
{
    AINameSpace *pns = g_manager.ns_head;
    while (pns) {
	if (strcmp(pns->name, ns) == 0)
	    return pns;
	pns = pns->next;
    }
    return NULL;
}

static AINode * m_get_node(const char *node, size_t len)
{
    char *p = malloc(len+1);
    if (p == NULL)
	return NULL;
    strncpy(p, node, len);

    AIZone *pz = g_manager.z_head;
    while (pz) {
	AINode *pn = pz->head;
	while (pn) {
	    if (strcmp(pn->ip_port, p) == 0) {
		free(p);
		return pn;
	    }
	    pn = pn->next;
	}
	pz = pz->next;
    }
    free(p);
    return NULL;
}

// private
static AIZone * m_create_zone(const char *name, int weight)
{
    AIManager * pm = &g_manager;

    AIZone *pz = (AIZone *)malloc(sizeof(AIZone));
    if (pz == NULL)
	return NULL;

    aiz_init(pz, name, weight);

    pz->pre = pm->z_tail;
    pz->next = NULL;
    if (pm->z_tail)
	pm->z_tail->next = pz;
    else
	pm->z_head = pz;
    pm->z_tail = pz;

    return pz;
}

// private
static ADFS_RESULT m_create_ns(const char *name)
{
    AIManager * pm = &g_manager;
    AINameSpace *pns = pm->ns_head;
    while (pns) {
    	if (strcmp(pns->name, name) == 0)
	    return ADFS_ERROR;
	pns = pns->next;
    }
    DBG_PRINTSN("m_create_ns 10");
    pns = malloc(sizeof(AINameSpace));
    if (pns == NULL) 
	return ADFS_ERROR;

    DBG_PRINTSN("m_create_ns 20");
    strncpy(pns->name, name, sizeof(pns->name));
    char indexdb_path[ADFS_MAX_PATH] = {0};
    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->path, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    DBG_PRINTSN(indexdb_path);
    pns->index_db = kcdbnew();
    if (kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE) == 0) 
        return ADFS_ERROR;
    DBG_PRINTSN("m_create_ns 30");

    pns->pre = pm->ns_tail;
    pns->next = NULL;
    if (pm->ns_tail)
	pm->ns_tail->next = pns;
    else
	pm->ns_head = pns;
    pm->ns_tail = pns;

    return ADFS_OK;
}

static char * m_get_history(const char *line, int order)
{
    int len = strlen(line);
    char *record;
    int count = 0, size;
    int pos_cur=len-1, pos_tail=len-1, pos_head=0;

    for (pos_cur=len-1; pos_cur>=0; --pos_cur) {
	if (line[pos_cur] == '$') {
	    ++count;
	    if (count == order)
		pos_tail = pos_cur-1;
	    if (count == order+1)
		pos_head = pos_cur+1;
	}
    }
    if (count < order)
	return NULL;

    size = pos_tail - pos_head + 1;
    record = malloc(size+1);
    if (record == NULL)
	return NULL;
    memset(record, 0, size+1);
    strncpy(record, &(line[pos_head]), size);

    return record;
}

static ADFS_RESULT m_erase()
{
    AINameSpace *pns = g_manager.ns_head;
    while (pns) {
	e_traverse(pns);
	pns = pns->next;
    }
    return ADFS_OK;
}

static ADFS_RESULT e_traverse(AINameSpace *pns)
{
    KCCUR *cur;
    char *kbuf;
    const char *cvbuf;
    size_t ksiz, vsiz;

    cur = kcdbcursor(pns->index_db);
    kccurjump(cur);
    while ((kbuf = kccurget(cur, &ksiz, &cvbuf, &vsiz, 0)) != NULL) {
	const char *hold = e_parse(pns->name, kbuf, cvbuf);
	if (strlen(hold) > 0)
	    kccursetvalue(cur, hold, strlen(hold), 1);
	else
	    kccurremove(cur);
	kcfree(kbuf);
    }
    kccurdel(cur);
    return ADFS_OK;
}

static const char * e_parse(const char *ns, const char *fname, const char *record)
{
    const char *rest = record;
    const char *pos_dollar = NULL;

    while ((pos_dollar = strstr(rest, "$"))) {
	if (e_connect(ns, fname, rest, pos_dollar - rest) == ADFS_ERROR)
	    break;
	rest = pos_dollar +1;
    }
    return rest;
}

static ADFS_RESULT e_connect(const char *ns, const char *fname, const char *record, int len)
{
    char *r = malloc(len+1);
    if (r == NULL)
	return ADFS_ERROR;
    memset(r, 0, len+1);
    strncpy(r, record, len);

    char *rest = r;
    while (rest) {
	AINode *pn = NULL;
	char *pos_sharp = strstr(rest, "#");
	char *pos_split = strstr(pos_sharp, "|");
	rest = pos_split;

	if (pos_split)
	    pn = m_get_node(pos_sharp +1, pos_split - pos_sharp -1);
	else 
	    pn = m_get_node(pos_sharp +1, strlen(pos_sharp +1));

	if (pn != NULL) {
	    char url[ADFS_MAX_PATH] = {0};
	    if (ns) {
		snprintf(url, sizeof(url), "http://%s/erase/%s%.*s?namespace=%s", 
			pn->ip_port, fname, ADFS_UUID_LEN, r, ns);
	    }
	    else {
		snprintf(url, sizeof(url), "http://%s/erase/%s%.*s", pn->ip_port, fname, ADFS_UUID_LEN, r);
	    }
	    aic_erase(pn, url);		// do not care about success or failure.
	}
    }

    free(r);
    return ADFS_OK;
}

