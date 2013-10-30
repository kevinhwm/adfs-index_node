/* manager.c
 *
 * kevinhwm@gmail.com
 */

#include <string.h>
#include <time.h>
#include <dirent.h>     // opendir
#include <unistd.h>
#include <sys/stat.h>   // chmod
#include <kclangc.h>
#include <curl/curl.h>

#include "manager.h"
#include "record.h"
#include "../cJSON.h"

static ADFS_RESULT m_init_log(cJSON *json);
static ADFS_RESULT m_init_stat(cJSON *json);
static ADFS_RESULT m_init_ns(cJSON *json);
static ADFS_RESULT m_init_zone(cJSON *json);

static AIZone * m_create_zone(const char *name, int weight);
static ADFS_RESULT m_create_ns(const char *name);
static AINameSpace * m_get_ns(const char *ns);
static AINode * m_get_node(const char *node_name, size_t len);
static char * m_get_history(const char *, int);
static AIZone * m_choose_zone(const char * record);

AIManager g_manager;
unsigned long g_MaxFileSize;
LOG_LEVEL g_log_level = LOG_LEVEL_DEBUG;
int g_another_running = 0;

ADFS_RESULT aim_init(const char *conf_file, const char *data_dir, long bnum, unsigned long mem_size, unsigned long max_file_size)
{
    srand(time(NULL));

    char msg[1024] = {0};
    AIManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));

    char f_flag[1024] = {0};
    snprintf(f_flag, sizeof(f_flag), "adfs.flag");
    if (access(f_flag, F_OK) != -1) {
	snprintf(msg, sizeof(msg), "[%s]->Another instance is running.", f_flag);
	log_out("manager", msg, LOG_LEVEL_FATAL);
	g_another_running = 1;
	return ADFS_ERROR;
    }
    else {
	time_t t;
	char stime[64] = {0};
	time(&t);
	FILE *f = fopen(f_flag, "wb+");
	if (f == NULL) {
	    snprintf(msg, sizeof(msg), "Can not create file.");
	    log_out("manager", msg, LOG_LEVEL_FATAL);
	    return ADFS_ERROR;
	}
	fprintf(f, "%s", ctime_r(&t, stime));
	fclose(f);
    }

    if (aiu_init() < 0) {return ADFS_ERROR;}

    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = bnum;
    pm->kc_msiz = mem_size *1024*1024;
    strncpy(pm->data_dir, "data", sizeof(pm->data_dir));
    strncpy(pm->log_dir, "log", sizeof(pm->log_dir));

    cJSON *json = conf_parse(conf_file);
    if (json == NULL) {
	log_out("manager", "Config file error.", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    if (m_init_log(json) == ADFS_ERROR) {goto err1;}
    if (m_init_ns(json) == ADFS_ERROR) {goto err1;}
    if (m_init_zone(json) == ADFS_ERROR) {goto err1;}
    if (m_init_stat(json) == ADFS_ERROR) {goto err1;}

    g_MaxFileSize = max_file_size *1024*1024;
    curl_global_init(CURL_GLOBAL_ALL);
    conf_release(json);
    return ADFS_OK;
err1:
    conf_release(json);
    return ADFS_ERROR;
}

void aim_exit()
{
    AIManager *pm = &g_manager;
    AIZone *pz = pm->z_head;
    while (pz) {
        AIZone *tmp = pz;
        pz = pz->next;
        tmp->release(tmp);
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
    if (pm->s_upload.release) {pm->s_upload.release(&(pm->s_upload));}
    if (pm->s_download.release) {pm->s_download.release(&(pm->s_download));}
    if (pm->s_delete.release) {pm->s_delete.release(&(pm->s_delete));}
    log_release();
    curl_global_cleanup();
    if (!g_another_running) {
	char f_flag[1024] = {0};
	snprintf(f_flag, sizeof(f_flag), "%s/adfs.flag", pm->data_dir);
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
    if (name_space == NULL) {name_space = "default";}

    AINameSpace *pns = NULL;
    pns = m_get_ns(name_space);
    if (pns == NULL) {return ADFS_ERROR;}
    // (1) need to be released
    old_list = kcdbget(pns->index_db, fname, strlen(fname), &old_list_len);
    if (old_list == NULL || old_list[old_list_len-1] == '$') {exist = 0;}
    if (exist && !overwrite) {goto ok1;} // exist and not overwrite

    AIPosition *pp;	// may be used in "rollback" tag
    // (2) need to be released
    AIRecord air;
    air_init(&air);

    AIZone *pz = pm->z_head;
    while (pz) {
	AINode * pn = pz->rand_choose(pz);
	char url[ADFS_MAX_LEN] = {0};
	snprintf(url, sizeof(url), "http://%s/upload_file/%s%.*s?namespace=%s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid, name_space);
	if (aic_upload(pn, url, fname, fdata, fdata_len) == ADFS_ERROR) {
	    printf("upload error: %s\n", url);
	    goto rollback;
	}
	air.add(&air, pz->name, pn->name);
	pz = pz->next;
    }

    // add record
    // (3) need to be released
    char *record = air.get_string(&air);
    if (record == NULL) {goto err1;}
    if (old_list == NULL) {kcdbset(pns->index_db, fname, strlen(fname), record, strlen(record));}
    else {
	long len = strlen(record) + 2;
	// (4) need to be released
	char *new_list = malloc(len);
	if (new_list == NULL)  {goto err2;}

	if (exist) {snprintf(new_list, len, "$%s", record);}
	else {snprintf(new_list, len, "%s", record);}
	kcdbappend(pns->index_db, fname, strlen(fname), new_list, strlen(new_list));
	if (new_list) {free(new_list);}
    }
    pm->s_upload.inc(&(pm->s_upload));

    if (record) {free(record);}
    air.release(&air);
ok1:
    if (old_list) {kcfree(old_list);}
    return ADFS_OK;

rollback:
    pp = air.head;
    while (pp) {
	char *pos_sharp = strstr(pp->zone_node, "#");
	AINode *pn = m_get_node(pos_sharp + 1, strlen(pos_sharp +1));
	if (pn != NULL) {
	    char url[ADFS_MAX_LEN] = {0};
	    snprintf(url, sizeof(url), "http://%s/erase/%s%.*s?namespace=%s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid, name_space);
	    aic_connect(pn, url, FLAG_ERASE);
	}
	pp = pp->next;
    }
    goto err1;
err2:
    if (record) {free(record);}
err1:
    air.release(&air);
    if (old_list) {kcfree(old_list);}
    return ADFS_ERROR;
}

// return value:
// NULL:    file not found
// url :    "char *" must be freed by caller
char * aim_download(const char *ns, const char *fname, const char *history)
{
    AIManager *pm = &g_manager;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    AINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) {return NULL;}

    int order = 0;
    if (history != NULL) {
	for (int i=0; i<strlen(history); ++i) {
	    if (history[i] < '0' || history[i] > '9') { return NULL; }
	}
	if ((order = atoi(history)) < 0) {return NULL;}
    }

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) {return NULL;}
    char *url = (char *)malloc(ADFS_MAX_LEN);
    if (url == NULL) {goto err1;}
    memset(url, 0, ADFS_MAX_LEN);
    char *record = m_get_history(line, order);
    if (record == NULL) {goto err2;}

    AIZone *pz = m_choose_zone(record + ADFS_UUID_LEN);
    if (pz == NULL) {goto err3;}
    char *pos_zone = strstr(record, pz->name);
    char *pos_sharp = strstr(pos_zone, "#");
    char *pos_split = strstr(pos_sharp, "|");
    if (pos_split) {
	char node_name[ADFS_FILENAME_LEN] = {0};
	strncpy(node_name, pos_sharp+1, (int)(pos_split-pos_sharp-1));
	AINode *pn = m_get_node(node_name, strlen(node_name));
	if (pn == NULL) {goto err3;}
	snprintf(url, ADFS_MAX_LEN, "http://%s/download/%s%.*s", pn->ip_port, fname, ADFS_UUID_LEN, record);
    }
    else {
	AINode *pn = m_get_node(pos_sharp+1, strlen(pos_sharp+1));
	if (pn == NULL) {goto err3;}
	snprintf(url, ADFS_MAX_LEN, "http://%s/download/%s%.*s", pn->ip_port, fname, ADFS_UUID_LEN, record);
    }
    strncat(url, "?namespace=", ADFS_MAX_LEN);
    strncat(url, pns->name, ADFS_MAX_LEN);
    pm->s_download.inc(&(pm->s_download));

    if (record) {free(record);}
    if (line) {kcfree(line);}
    return url;
err3:
    if (record) {free(record);}
err2:
    if (url) {free(url);}
err1:
    if (line) {kcfree(line);}
    return NULL;
}

ADFS_RESULT aim_delete(const char *ns, const char *fname)
{
    AIManager *pm = &g_manager;
    const char *name_space = ns;
    if (name_space == NULL) {name_space = "default";}
    AINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) {return ADFS_ERROR;}

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) {return ADFS_ERROR;}
    if (line[vsize-1] == '$') {goto ok1;}

    char *new_line = malloc(vsize+4);
    if (new_line == NULL) {goto err1;}
    memset(new_line, 0, vsize+4);
    strcpy(new_line, line);
    strcat(new_line, "$");
    kcdbset(pns->index_db, fname, strlen(fname), new_line, strlen(new_line));
    pm->s_delete.inc(&(pm->s_delete));
    free(new_line);
ok1:
    kcfree(line);
    return ADFS_OK;
err1:
    kcfree(line);
    return ADFS_ERROR;
}

int aim_exist(const char *name_space, const char *fname)
{
    if (name_space == NULL) {name_space = "default";}
    AINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) {return 0;}

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) {return 0;}
    else if (line[vsize-1] == '$') {kcfree(line); return 0;}
    else {kcfree(line); return 1;}
}


char * aim_status()
{
    int size = 1024 * 32;
    char *p = malloc(size);
    if (p == NULL) {return NULL;}
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
	    if (aic_connect(pn, url, FLAG_STATUS) == ADFS_OK) {strncat(p, "bgcolor=\"green\"><font color=\"white\">alive</font>", size);}
	    else {strncat(p, "bgcolor=\"red\"><font color=\"white\">lost</font>", size);}
	    strncat(p, "</td></tr>", size);
	    pn = pn->next;
	}
	strncat(p, "</table>", size);
    	pz = pz->next;
    }

    /*
    time_t t_cur = time(NULL);
    struct tm *lt = localtime(&t_cur);
    char str_time[32] = {0};
    strftime(str_time, sizeof(str_time), "current time: %H:%M", lt);
    strncat(p, "<p>", size);
    strncat(p, str_time, size);
    strncat(p, "</p>", size);
    int *pcount = pm->s_upload.get(&(pm->s_upload), &t_cur);
    for (int i=0; i<pm->s_upload.scope; ++i) {
	if (pcount < pm->s_upload.count) {pcount += pm->s_upload.scope;}
	char tmp[128] = {0};
	snprintf(tmp, sizeof(tmp), "%d", *pcount);
	strncat(p, tmp, size);
	strncat(p, "|", size);
	if ((i+1)%10 == 0) {strncat(p, "<br />", size);}
	pcount--;
    }
    */

    return p;
}

static ADFS_RESULT m_init_zone(cJSON *json)
{
    char msg[1024];
    cJSON *j_tmp = cJSON_GetObjectItem(json, "zone");
    if (j_tmp && (j_tmp = j_tmp->child)) {
	while (j_tmp) {
	    cJSON *j_name = cJSON_GetObjectItem(j_tmp, "name");
	    cJSON *j_weight = cJSON_GetObjectItem(j_tmp, "weight");
	    AIZone *pz = m_create_zone(j_name->valuestring, j_weight->valueint);
	    if (pz == NULL) {
		snprintf(msg, sizeof(msg), "[%s]->create zone error", j_name->valuestring);
		log_out("manager", msg, LOG_LEVEL_ERROR);
		return ADFS_ERROR;
	    }

	    cJSON *node_list = cJSON_GetObjectItem(j_tmp, "node");
	    cJSON *node = node_list->child;
	    while (node) {
		if (pz->create(pz, node->string, node->valuestring) == ADFS_ERROR) {
		    snprintf(msg, sizeof(msg), "[%s]->create node error", node->string);
		    log_out("manager", msg, LOG_LEVEL_ERROR);
		    return ADFS_ERROR;
		}
		node = node->next;
	    }
	    j_tmp = j_tmp->next;
	}
    }
    return ADFS_OK;
}

static ADFS_RESULT m_init_ns(cJSON *json)
{
    char msg[1024];
    cJSON *j_tmp = NULL;

    j_tmp = cJSON_GetObjectItem(json, "namespace");
    if (j_tmp && (j_tmp = j_tmp->child)) {
	while (j_tmp) {
	    if (strlen(j_tmp->valuestring) == 0) {
		snprintf(msg, sizeof(msg), "[namespace]->config file error");
		log_out("manager", msg, LOG_LEVEL_ERROR);
		return ADFS_ERROR;
	    }
	    if (m_create_ns(j_tmp->valuestring) == ADFS_ERROR) {
		snprintf(msg, sizeof(msg), "[%s]->create namespace error", j_tmp->valuestring);
		log_out("manager", msg, LOG_LEVEL_ERROR);
		return ADFS_ERROR;
	    }
	    j_tmp = j_tmp->next;
	}
    }
    
    if (m_create_ns("default") == ADFS_ERROR) {
	snprintf(msg, sizeof(msg), "[default]->create namespace error");
	log_out("manager", msg, LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    return ADFS_OK;
}

static ADFS_RESULT m_init_log(cJSON *json)
{
    // log_level
    cJSON *j_tmp = NULL;
    j_tmp = cJSON_GetObjectItem(json, "log_level");
    if (j_tmp == NULL) {
	log_out("manager", "[log_level]->config file error", LOG_LEVEL_SYSTEM);
        return ADFS_ERROR;
    }
    g_log_level = j_tmp->valueint;
    if (g_log_level < 1 || g_log_level > 5) {
	log_out("manager", "[log_level]->config value error", LOG_LEVEL_SYSTEM);
	return ADFS_ERROR;
    }

    // log_file
    j_tmp = cJSON_GetObjectItem(json, "log_file");
    if (j_tmp == NULL) { 
	log_out("manager", "[log_file]->config file error", LOG_LEVEL_SYSTEM);
	return ADFS_ERROR;
    }
    if (log_init(j_tmp->valuestring) != 0) {
	log_out("manager", "[log_file]->config value error", LOG_LEVEL_SYSTEM);
	return ADFS_ERROR;
    }
    return ADFS_OK;
}

static ADFS_RESULT m_init_stat(cJSON *json)
{
    AIManager *pm = &g_manager;
    // statistics
    cJSON *j_tmp = NULL;
    j_tmp = cJSON_GetObjectItem(json, "stat_hour");
    if (j_tmp == NULL) {
	log_out("manager", "[stat_hour]->config file error", LOG_LEVEL_ERROR);
	return ADFS_ERROR;
    }
    long stat_hour = j_tmp->valueint;
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

static AINameSpace * m_get_ns(const char *ns)
{
    AINameSpace *pns = g_manager.ns_head;
    while (pns) {
	if (strcmp(pns->name, ns) == 0) {return pns;}
	pns = pns->next;
    }
    return NULL;
}

static AINode * m_get_node(const char *node_name, size_t len)
{
    char *p = malloc(len+1);
    if (p == NULL) {return NULL;}
    p[len] = '\0';
    strncpy(p, node_name, len);

    AIZone *pz = g_manager.z_head;
    while (pz) {
	AINode *pn = pz->head;
	while (pn) {
	    if (strcmp(pn->name, p) == 0) {free(p); return pn;}
	    pn = pn->next;
	}
	pz = pz->next;
    }
    free(p);
    return NULL;
}

static AIZone * m_create_zone(const char *name, int weight)
{
    AIManager * pm = &g_manager;
    AIZone *pz = pm->z_head;
    for (; pz; pz=pz->next) {
	if (strcmp(pz->name, name) == 0 ) {return NULL;} 
    }
    pz = (AIZone *)malloc(sizeof(AIZone));
    if (pz == NULL) {return NULL;}
    aiz_init(pz, name, weight);
    pz->prev = pm->z_tail;
    pz->next = NULL;
    if (pm->z_tail) {pm->z_tail->next = pz;}
    else {pm->z_head = pz;}
    pm->z_tail = pz;
    return pz;
}

static ADFS_RESULT m_create_ns(const char *name)
{
    AIManager * pm = &g_manager;
    AINameSpace *pns = pm->ns_head;
    while (pns) {
    	if (strcmp(pns->name, name) == 0) {return ADFS_ERROR;}
	pns = pns->next;
    }
    pns = malloc(sizeof(AINameSpace));
    if (pns == NULL) {return ADFS_ERROR;}

    strncpy(pns->name, name, sizeof(pns->name));
    char indexdb_path[ADFS_MAX_LEN] = {0};
    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->data_dir, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    pns->index_db = kcdbnew();
    if (kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK) == 0) {return ADFS_ERROR;}

    pns->prev = pm->ns_tail;
    pns->next = NULL;
    if (pm->ns_tail) {pm->ns_tail->next = pns;}
    else {pm->ns_head = pns;}
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
	    if (count == order) {pos_tail = pos_cur-1;}
	    if (count == order+1) {pos_head = pos_cur+1;}
	}
    }
    if (count < order) {return NULL;}
    size = pos_tail - pos_head + 1;
    record = malloc(size+1);
    if (record == NULL) {return NULL;}
    memset(record, 0, size+1);
    strncpy(record, &(line[pos_head]), size);
    return record;
}

static AIZone * m_choose_zone(const char *record)
{
    if (record == NULL) {return NULL;}
    AIManager *pm = &g_manager;
    AIZone *biggest_z = NULL;   // (weight/count)
    AIZone *least_z = NULL;

    AIZone *pz = pm->z_head;
    for (; pz; pz = pz->next) {
        if (strstr(record, pz->name) == NULL) {continue;}

        if (pz->count == 0) {pz->count += 1; return pz;}
        else if (biggest_z == NULL) {biggest_z = pz;}
        else {
            biggest_z = (pz->weight / pz->count) > (biggest_z->weight / biggest_z->count) ? pz : biggest_z;
            if (least_z == NULL) {least_z = pz;}
            else {least_z = (pz->weight / pz->count) < (least_z->weight / least_z->count) ? pz : least_z;}
        }
    }
    if (biggest_z) {
	biggest_z->count += 1;
	if (biggest_z == least_z) {
	    pz = pm->z_head;
	    for (; pz; pz = pz->next) {pz->count = 0;}
	}
    }
    return biggest_z;
}
