/* manager.c
 *
 * kevinhwm@gmail.com
 */

#include <string.h>
#include <time.h>
#include <unistd.h>
#include <kclangc.h>
#include <curl/curl.h>

#include "manager.h"
#include "meta.h"
#include "../cJSON.h"


static int m_init_log(cJSON *json);				// initialize
static int m_init_ns(cJSON *json);				// initialize
static int m_init_zone(cJSON *json);				// initialize
static CIZone * m_create_zone(const char *name, int weight);	// initialize
static int m_create_ns(const char *name);			// initialize

static CINameSpace * m_get_ns(const char *ns);			// dynamic no write - upload download delete exist
static CINode * m_get_node(const char *node_name, size_t len);	// dynamic no write - upload download

static char * m_get_history(const char *, int);			// dynamic - download
static CIZone * m_choose_zone(const char * record);		// dynamic - download


CIManager g_manager;

int GIm_init(const char *conf_file, long bnum, unsigned long mem_size, unsigned long max_file_size)
{
    CIManager *pm = &g_manager;
    memset(pm, 0, sizeof(CIManager));

    char *f_flag = _DFS_RUNNING_FLAG;	// running.flag
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
    pm->kc_bnum = bnum;
    pm->kc_msiz = mem_size *1024*1024;
    pm->max_file_size = max_file_size *1024*1024;
    strncpy(pm->data_dir, "data", sizeof(pm->data_dir));
    strncpy(pm->log_dir, "log", sizeof(pm->log_dir));
    sprintf(pm->core_log, "%s/aicore.log", pm->log_dir);

    if (GIu_run() < 0) { return -1; }

    cJSON *json = conf_parse(conf_file);
    if (json == NULL) { return -1; }
    if (m_init_log(json) < 0) { conf_release(json); return -1; }
    if (m_init_ns(json) < 0) { conf_release(json); return -1; }
    if (m_init_zone(json) < 0) { conf_release(json); return -1; }
    conf_release(json);

    curl_global_init(CURL_GLOBAL_ALL);
    srand(time(NULL));
    return 0;
}

int GIm_exit()
{
    CIManager *pm = &g_manager;
    if (pm->another_running) { return 0; }

    CIZone *pz = pm->z_head;
    while (pz) {
	CIZone *tmp = pz;
	pz = pz->next;
	tmp->release(tmp);
	free(tmp);
    }
    CINameSpace *pns = pm->ns_head;
    while (pns) {
	CINameSpace *tmp = pns;
	pns = pns->next;
	kcdbclose(tmp->index_db);
	kcdbdel(tmp->index_db);
	free(tmp);
    }
    log_release();
    curl_global_cleanup();
    remove(_DFS_RUNNING_FLAG); 
    return 0;
}

int GIm_upload(const char *ns, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    CIManager *pm = &g_manager;
    int exist = 1;
    char *old_line = NULL;
    size_t old_line_len = 0 ;
    const char *name_space = ns;
    if (name_space == NULL) { name_space = "default"; }

    CINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) { return -1; }

    // (1) need to be released
    old_line = kcdbget(pns->index_db, fname, strlen(fname), &old_line_len);
    if (old_line == NULL || old_line[old_line_len-1] == '$') { exist = 0; }
    if (old_line) { kcfree(old_line); old_line = NULL;}
    if (exist && !overwrite) { goto ok1; } 		// exist and not overwrite

    CIPosition *pp;		// may be used in "rollback" tag
    // (2) need to be released
    CIFile a_file;
    GIf_init(&a_file);

    CIZone *pz = pm->z_head;
    while (pz) {
	CINode * pn = pz->rand_choose(pz);
	char url[_DFS_MAX_LEN] = {0};
	snprintf(url, sizeof(url), "http://%s/upload_file/%s%.*s?namespace=%s", pn->ip_port, fname, _DFS_UUID_LEN, a_file.uuid, name_space);
	if (GIc_upload(pn, url, fname, fdata, fdata_len) < 0) {
	    printf("upload error: %s\n", url);
	    goto rollback;
	}
	a_file.add(&a_file, pz->name, pn->name);
	pz = pz->next;
    }

    // add record
    // (3) need to be released
    char *record = a_file.get_string(&a_file);
    if (record == NULL) { goto err1; }
    size_t len = strlen(record) + 2;
    // (4) need to be released
    char *new_list = malloc(len);
    if (new_list == NULL)  { goto err2; }

    snprintf(new_list, len, "$%s", record);
    int res = kcdbappend(pns->index_db, fname, strlen(fname), new_list, strlen(new_list));

    if (new_list) { free(new_list); }
    if (record) { free(record); }
    a_file.release(&a_file);

    if (!res) { return -1; }
ok1:
    return 0;

rollback:
    pp = a_file.head;
    while (pp) {
	char *pos_sharp = strstr(pp->zone_node, "#");
	CINode *pn = m_get_node(pos_sharp + 1, strlen(pos_sharp +1));
	if (pn != NULL) {
	    char url[_DFS_MAX_LEN] = {0};
	    snprintf(url, sizeof(url), "http://%s/erase/%s%.*s?namespace=%s", pn->ip_port, fname, _DFS_UUID_LEN, a_file.uuid, name_space);
	    GIc_connect(pn, url, FLAG_ERASE);
	}
	pp = pp->next;
    }
    goto err1;
err2:
    if (record) { free(record); }
err1:
    a_file.release(&a_file);
    return -1;
}

char * GIm_download(const char *ns, const char *fname, const char *history)
{
    const char *name_space = ns;
    if (name_space == NULL) { name_space = "default"; }
    CINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) { return NULL; }

    int order = 0;
    if (history != NULL) {
	for (int i=0; i<strlen(history); ++i) {
	    if (history[i] < '0' || history[i] > '9') { return NULL; }
	}
	if ((order = atoi(history)) < 0) { return NULL; }
    }

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) {return NULL;}
    char *url = (char *)malloc(_DFS_MAX_LEN);
    if (url == NULL) { goto err1; }
    memset(url, 0, _DFS_MAX_LEN);

    char *record = m_get_history(line, order);
    if (record == NULL) { goto err2; }

    CIZone *pz = m_choose_zone(record + _DFS_UUID_LEN);
    if (pz == NULL) { goto err3; }
    char *pos_zone = strstr(record, pz->name);
    char *pos_sharp = strstr(pos_zone, "#");
    char *pos_split = strstr(pos_sharp, "|");
    if (pos_split) {
	char node_name[_DFS_FILENAME_LEN] = {0};
	strncpy(node_name, pos_sharp+1, (int)(pos_split-pos_sharp-1));
	CINode *pn = m_get_node(node_name, strlen(node_name));
	if (pn == NULL) { goto err3; }
	snprintf(url, _DFS_MAX_LEN, "http://%s/download/%s%.*s", pn->ip_port, fname, _DFS_UUID_LEN, record);
    }
    else {
	CINode *pn = m_get_node(pos_sharp+1, strlen(pos_sharp+1));
	if (pn == NULL) { goto err3; }
	snprintf(url, _DFS_MAX_LEN, "http://%s/download/%s%.*s", pn->ip_port, fname, _DFS_UUID_LEN, record);
    }
    strncat(url, "?namespace=", _DFS_MAX_LEN);
    strncat(url, pns->name, _DFS_MAX_LEN);

    if (record) { free(record); }
    if (line) { kcfree(line); }
    return url;
err3:
    if (record) { free(record); }
err2:
    if (url) { free(url); }
err1:
    if (line) { kcfree(line); }
    return NULL;
}

int GIm_exist(const char *name_space, const char *fname)
{
    if (name_space == NULL) { name_space = "default"; }
    CINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) { return 0; }

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) { return 0; }
    else if (line[vsize-1] == '$') { kcfree(line); return 0; }
    else { kcfree(line); return 1; }
}

int GIm_delete(const char *ns, const char *fname)
{
    const char *name_space = ns;
    if (name_space == NULL) { name_space = "default"; }
    CINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) { return -1; }

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) { return -1; }
    if (line[vsize-1] == '$') {goto ok1;}
    if ( !kcdbappend(pns->index_db, fname, strlen(fname), "$", 1) ) { goto err1; };
ok1:
    kcfree(line);
    return 0;
err1:
    kcfree(line);
    return -1;
}

char * GIm_status()
{
    int size = 1024 * 32;
    char *p = malloc(size);
    if (p == NULL) {return NULL;}
    memset(p, 0, size);

    CIManager *pm = &g_manager;
    CIZone *pz = pm->z_head;
    while (pz) {
	strncat(p, "<li><font color=\"blue\">Zone: ", size);
	strncat(p, pz->name, size);
	strncat(p, "</font></li><table border=\"1\">\n", size);
	strncat(p, "<tr><th>node</th><th>status</th></tr>\n", size);
	CINode *pn = pz->head;
	while (pn) {
	    strncat(p, "<tr><td>", size);
	    strncat(p, pn->ip_port, size);
	    strncat(p, "</td><td ", size);
	    char url[1024] = {0};
	    snprintf(url, sizeof(url), "http://%s/status", pn->ip_port);
	    if (GIc_connect(pn, url, FLAG_STATUS) >= 0) {strncat(p, "bgcolor=\"green\"><font color=\"white\">alive</font>", size);}
	    else {strncat(p, "bgcolor=\"red\"><font color=\"white\">lost</font>", size);}
	    strncat(p, "</td></tr>", size);
	    pn = pn->next;
	}
	strncat(p, "</table>", size);
	pz = pz->next;
    }

    return p;
}

static int m_init_log(cJSON *json)
{
    cJSON *j_tmp = cJSON_GetObjectItem(json, "log_level");
    if (j_tmp == NULL) {
	fprintf(stderr, "[log_level]->config file error\n");
	return -1;
    }
    LOG_LEVEL log_level = j_tmp->valueint;
    if (log_init(log_level) < 0) {
	fprintf(stderr, "[log_level]->config value error\n");
	return -1;
    }
    return 0;
}

static int m_init_ns(cJSON *json)
{
    cJSON *j_tmp = cJSON_GetObjectItem(json, "namespace");
    if (j_tmp && (j_tmp = j_tmp->child)) {
	while (j_tmp) {
	    size_t len = strlen(j_tmp->valuestring);
	    if (len == 0 || len >= _DFS_NAMESPACE_LEN) {
		fprintf(stderr, "[namespace]->create file error\n");
		return -1;
	    }
	    if (m_create_ns(j_tmp->valuestring) < 0) {
		fprintf(stderr, "[%s]->create namespace error\n", j_tmp->valuestring);
		return -1;
	    }
	    j_tmp = j_tmp->next;
	}
    }
    if (m_create_ns("default") < 0) {
	fprintf(stderr, "[default]->create namespace error\n");
	return -1;
    }
    return 0;
}

static int m_init_zone(cJSON *json)
{
    cJSON *j_tmp = cJSON_GetObjectItem(json, "zone");
    if (j_tmp && (j_tmp = j_tmp->child)) {
	while (j_tmp) {
	    cJSON *j_name = cJSON_GetObjectItem(j_tmp, "name");
	    cJSON *j_weight = cJSON_GetObjectItem(j_tmp, "weight");
	    CIZone *pz = m_create_zone(j_name->valuestring, j_weight->valueint);
	    if (pz == NULL) {
		fprintf(stderr, "[%s]->create zone error\n", j_name->valuestring);
		return -1;
	    }

	    cJSON *node_list = cJSON_GetObjectItem(j_tmp, "node");
	    cJSON *node = node_list->child;
	    while (node) {
		cJSON *one_attr = node->child;
		char *name = one_attr->valuestring;
		char *state = one_attr->next->valuestring;
		char *ip_port = one_attr->next->next->valuestring;
		if (!name || !state || !ip_port || pz->create(pz, name, state, ip_port) < 0) {
		    fprintf(stderr, "[%s]->create node error\n", node->string);
		    return -1;
		}
		node = node->next;
	    }
	    j_tmp = j_tmp->next;
	}
    }
    return 0;
}

static CIZone * m_create_zone(const char *name, int weight)
{
    CIManager * pm = &g_manager;
    CIZone *pz = pm->z_head;
    for (; pz; pz=pz->next) {
	if (strcmp(pz->name, name) == 0 ) {return NULL;} 
    }
    pz = (CIZone *)malloc(sizeof(CIZone));
    if (pz == NULL) {return NULL;}
    GIz_init(pz, name, weight);
    pz->prev = pm->z_tail;
    pz->next = NULL;
    if (pm->z_tail) {pm->z_tail->next = pz;}
    else {pm->z_head = pz;}
    pm->z_tail = pz;
    return pz;
}

static int m_create_ns(const char *name)
{
    CIManager * pm = &g_manager;
    CINameSpace *pns = pm->ns_head;
    while (pns) {
	if (strcmp(pns->name, name) == 0) { return -1; }
	pns = pns->next;
    }
    pns = malloc(sizeof(CINameSpace));
    if (pns == NULL) { return -1; }
    memset(pns, 0, sizeof(CINameSpace));

    strncpy(pns->name, name, sizeof(pns->name));
    char indexdb_path[_DFS_MAX_LEN] = {0};
    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
	    pm->data_dir, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    pns->index_db = kcdbnew();
    if (kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK) == 0) {
	free(pns);
	return -1;
    }

    pns->prev = pm->ns_tail;
    pns->next = NULL;
    if (pm->ns_tail) {pm->ns_tail->next = pns;}
    else {pm->ns_head = pns;}
    pm->ns_tail = pns;

    return 0;
}

static CINameSpace * m_get_ns(const char *ns)
{
    CINameSpace *pns = g_manager.ns_head;
    while (pns) {
	if (strcmp(pns->name, ns) == 0) { return pns; }
	pns = pns->next;
    }
    return NULL;
}

static CINode * m_get_node(const char *node_name, size_t len)
{
    char *p = malloc(len+1);
    if (p == NULL) { return NULL; }
    memset(p, 0, len+1);
    strncpy(p, node_name, len);

    CIZone *pz = g_manager.z_head;
    while (pz) {
	CINode *pn = pz->head;
	while (pn) {
	    if (strcmp(pn->name, p) == 0) {free(p); return pn;}
	    pn = pn->next;
	}
	pz = pz->next;
    }
    free(p);
    return NULL;
}

static char * m_get_history(const char *line, int order)
{
    int len = strlen(line);
    char *record = NULL;
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

static CIZone * m_choose_zone(const char *record)
{
    if (record == NULL) {return NULL;}
    CIManager *pm = &g_manager;
    CIZone *biggest_z = NULL;   // (weight/count)
    CIZone *least_z = NULL;

    CIZone *pz = pm->z_head;
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

