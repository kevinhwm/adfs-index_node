/* manager.c
 *
 * kevinhwm@gmail.com
 */

#include <string.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <kclangc.h>

#include "manager.h"
#include "meta.h"
#include "../cJSON.h"
#include "../md5.h"


static int m_init_syn();
static int m_init_log(cJSON *json);				// initialize
static int m_init_ns(cJSON *json);				// initialize
static int m_init_zone(cJSON *json);				// initialize
static CIZone * m_create_zone(const char *name);		// initialize
static int m_create_ns(const char *name);			// initialize

static CINameSpace * m_get_ns(const char *ns);			// dynamic no write - upload download delete exist
static CINode * m_get_node(const char *node_name, size_t len);	// dynamic no write - upload download

static char * m_get_history(const char *, int);			// dynamic - download
static CINode * m_choose(const char * record);			// dynamic - download


CIManager g_manager;

int GIm_init(const char *conf_file, const char *syn_dir, int role, long bnum, unsigned long mem_size, unsigned long max_file_size)
{
    CIManager *pm = &g_manager;
    memset(pm, 0, sizeof(CIManager));

    char *f_flag = _DFS_RUNNING_FLAG;	// running.flag
    if (access(f_flag, F_OK) != -1) {
	fprintf(stderr, "-> another instance is running...\n-> exit\n");
	pm->another_running = 1;
	return -1;
    }
    else {
	FILE *f = fopen(f_flag, "w");
	if (f == NULL) { 
	    fprintf(stderr, "-> create running-flag file error\n"); 
	    return -1; 
	}
	fclose(f);
	pm->another_running = 0;
    }

    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = bnum;
    pm->kc_msiz = mem_size *1024*1024;
    pm->max_file_size = max_file_size *1024*1024;
    pm->primary = role;
    if (syn_dir == NULL) { fprintf(stderr, "-> syn dir error\n"); return -1; }
    strncpy(pm->syn_dir, syn_dir, sizeof(pm->syn_dir));
    if (m_init_syn() < 0) { return -1; }

    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);

    if (GIu_run() < 0) { fprintf(stderr, "-> update data error\n"); return -1; }

    cJSON *json = conf_parse(conf_file);
    if (json == NULL) { fprintf(stderr, "-> parser config file error\n"); return -1; }
    if (m_init_log(json) < 0) { conf_release(json); return -1; }
    if (m_init_ns(json) < 0) { conf_release(json); return -1; }
    if (m_init_zone(json) < 0) { conf_release(json); return -1; }
    conf_release(json);

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
    /*
    GIsp_release();
    GIss_release();
    */
    curl_global_cleanup();
    return 0;
}

int GIm_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    CIManager *pm = &g_manager;
    int exist = 1;
    char *old_line = NULL;
    size_t old_line_len = 0 ;
    if (name_space == NULL) { name_space = "default"; }

    CINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) { return -1; }

    // (1) need to be released
    old_line = kcdbget(pns->index_db, fname, strlen(fname), &old_line_len);
    if (old_line == NULL || old_line[old_line_len-1] == '$') { exist = 0; }
    if (old_line) { kcfree(old_line); old_line = NULL;}
    if (exist && !overwrite) { return 0; } 		// exist and not overwrite

    // (2) need to be released
    CIFile a_file;
    GIf_init(&a_file);

    for (CIZone *pz = pm->z_head; pz; pz=pz->next) {
	int up_ok = 0;
	int node_num = 0;
	CINode ** node_list = pz->get_nodelist(pz, &node_num);
	char url[_DFS_MAX_LEN] = {0};
	for (int i=node_num; i; --i) {
	    int pos = rand()%i;
	    snprintf(url, sizeof(url), "http://%s/upload_file/%s%.*s?namespace=%s", 
		    node_list[pos]->ip_port, fname, _DFS_UUID_LEN, a_file.uuid, name_space);
	    if (GIc_upload(node_list[pos], url, fname, fdata, fdata_len) < 0) {
		CINode *tmp = node_list[i-1];
		node_list[i-1] = node_list[pos];
		node_list[pos] = tmp;
	    }
	    else {
		a_file.add(&a_file, pz->name, node_list[pos]->name);
		up_ok = 1;
		break;
	    }
	}
	free(node_list);
	if (!up_ok) { goto rollback; }
    }

    // (3) need to be released
    char *record = a_file.get_string(&a_file);
    if (record == NULL) { goto err1; }
    size_t len = strlen(record) + 2;
    // (4) need to be released
    char *new_list = malloc(len);
    if (new_list == NULL)  { goto err2; }

    snprintf(new_list, len, "$%s", record);
    int res = kcdbappend(pns->index_db, fname, strlen(fname), new_list, strlen(new_list));
    if (!res) { goto err3; }
    //if (!res || GIsp_export(pm->syn_dir, name_space, fname, new_list)<0) { goto err3; }

    if (new_list) { free(new_list); }
    if (record) { free(record); }
    a_file.release(&a_file);
    return 0;

rollback:
    for (CIPosition *pp = a_file.head; pp; pp = pp->next) {
	char *pos_sharp = strstr(pp->zone_node, "#");
	CINode *pn = m_get_node(pos_sharp+1, strlen(pos_sharp +1));
	if (pn != NULL) {
	    char url[_DFS_MAX_LEN] = {0};
	    snprintf(url, sizeof(url), "http://%s/erase/%s%.*s?namespace=%s", pn->ip_port, fname, _DFS_UUID_LEN, a_file.uuid, name_space);
	    GIc_connect(pn, url, FLAG_ERASE);
	}
    }
    goto err1;
err3:
    if (new_list) { free(new_list); }
err2:
    if (record) { free(record); }
err1:
    a_file.release(&a_file);
    return -1;
}

char * GIm_download(const char *name_space, const char *fname, const char *history)
{
    if (name_space == NULL) { name_space = "default"; }
    CINameSpace *pns = m_get_ns(name_space);
    if (pns == NULL) { return NULL; }

    int order = 0;
    if (history != NULL) {
	for (int i=0; i<strlen(history); ++i) {
	    if ( !isdigit(history[i]) ) { return NULL; }
	}
	if ((order =atoi(history)) < 0) { return NULL; }
    }

    size_t vsize;
    char *line = kcdbget(pns->index_db, fname, strlen(fname), &vsize);
    if (line == NULL) { return NULL; }
    char *url = (char *)malloc(_DFS_MAX_LEN);
    if (url == NULL) { goto err1; }
    memset(url, 0, _DFS_MAX_LEN);

    char *record = m_get_history(line, order);
    if (record == NULL) { goto err2; }

    CINode *pn = m_choose(record + _DFS_UUID_LEN);
    if (pn == NULL) { goto err3; }
    snprintf(url, _DFS_MAX_LEN, "http://%s/download/%s%.*s?namespace=%s", pn->ip_port, fname, _DFS_UUID_LEN, record, pns->name);
    printf("%s\n", url);

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

int GIm_delete(const char *name_space, const char *fname)
{
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

int GIm_setnode(const char *node_name, const char *attr_rw)
{
    CINode *pn = m_get_node(node_name, strlen(node_name));
    if (pn == NULL) {return -1;}

    pthread_mutex_lock(pn->lock);
    if (strcmp(attr_rw, "rw")==0) { pn->state = S_READ_WRITE; }
    else if (strcmp(attr_rw, "ro")==0) { pn->state = S_READ_ONLY; }
    else if (strcmp(attr_rw, "na")==0) { pn->state = S_NA; }
    else {
	pthread_mutex_unlock(pn->lock);
	return -1;
    }
    pthread_mutex_unlock(pn->lock);
    return 0;
}

char * GIm_status()
{
    int size = 1024 * 32;
    char *p = malloc(size);
    if (p == NULL) {return NULL;}
    memset(p, 0, size);

    CIManager *pm = &g_manager;
    for (CIZone *pz = pm->z_head; pz; pz = pz->next) {
	strncat(p, "<li><font color=\"blue\">Zone: ", size);
	strncat(p, pz->name, size);
	strncat(p, "</font></li><table border=\"1\">\n", size);
	strncat(p, "<tr><th>node</th><th>status</th></tr>\n", size);
	for (CINode *pn = pz->head; pn; pn = pn->next) {
	    strncat(p, "<tr><td>", size);
	    strncat(p, pn->ip_port, size);
	    strncat(p, "</td><td ", size);
	    char url[1024] = {0};
	    snprintf(url, sizeof(url), "http://%s/status", pn->ip_port);
	    if (GIc_connect(pn, url, FLAG_STATUS) >= 0) {strncat(p, "bgcolor=\"green\"><font color=\"white\">alive</font>", size);}
	    else {strncat(p, "bgcolor=\"red\"><font color=\"white\">lost</font>", size);}
	    strncat(p, "</td></tr>", size);
	}
	strncat(p, "</table>", size);
    }
    return p;
}

static int m_init_syn()
{
    // read/create local instance id
    CIManager *pm = &g_manager;
    unsigned char buf[16] = {0};
    unsigned char src[128] = {0};

    FILE *f_instance = fopen(MNGR_INSTANCE_F, "r");
    if (f_instance == NULL) {
	f_instance = fopen(MNGR_INSTANCE_F, "w");
	if (f_instance == NULL) { fprintf(stderr, "can not create instance id file\n"); return -1; }
	snprintf((char *)src, sizeof(src)-1, "%u%u", rand(), (unsigned int)time(NULL));
	md5(src, strlen((char *)src), buf);
	for (int i=0; i<16; ++i) {
	    sprintf(pm->instance_id+2*i, "%02x", buf[i]);
	}
	fwrite(pm->instance_id, 1, strlen(pm->instance_id), f_instance);
    }
    else { fread(pm->instance_id, 1, sizeof(pm->instance_id), f_instance); }
    fclose(f_instance);

    // read/create remote id
    if (pm->primary) {
	if (GIsp_init() < 0) { fprintf(stderr, "primary init error\n"); return -1; }
    }
    else {
	if (GIss_init() < 0) { fprintf(stderr, "secondary init error\n"); return -1; }
    }

    return 0;
}

static int m_init_log(cJSON *json)
{
    cJSON *j_tmp = cJSON_GetObjectItem(json, "log_level");
    if (j_tmp == NULL) {
	fprintf(stderr, "[log_level]->config file error\n");
	return -1;
    }
    LOG_LEVEL log_level = j_tmp->valueint;
    if (log_init(log_level, g_manager.instance_id) < 0) {
	fprintf(stderr, "[log_level]->config value error\n");
	return -1;
    }
    return 0;
}

static int m_init_ns(cJSON *json)
{
    if (m_create_ns("default") < 0) {
	fprintf(stderr, "[default]->create namespace error\n");
	return -1;
    }
    cJSON *j_tmp = cJSON_GetObjectItem(json, "namespace");
    if (j_tmp && (j_tmp = j_tmp->child)) {
	for (; j_tmp; j_tmp = j_tmp->next) {
	    size_t len = strlen(j_tmp->valuestring);
	    if (len == 0 || len >= _DFS_NAMESPACE_LEN) {
		fprintf(stderr, "[namespace]->create file error\n");
		return -1;
	    }
	    if (m_create_ns(j_tmp->valuestring) < 0) {
		fprintf(stderr, "[%s]->create namespace error\n", j_tmp->valuestring);
		return -1;
	    }
	}
    }

    return 0;
}

static int m_init_zone(cJSON *json)
{
    cJSON *j_tmp = cJSON_GetObjectItem(json, "zone");
    if (j_tmp && (j_tmp = j_tmp->child)) {
	for (; j_tmp; j_tmp = j_tmp->next) {
	    cJSON *j_name = cJSON_GetObjectItem(j_tmp, "name");
	    CIZone *pz = m_create_zone(j_name->valuestring);
	    if (pz == NULL) {
		fprintf(stderr, "[%s]->create zone error\n", j_name->valuestring);
		return -1;
	    }

	    cJSON *node_list = cJSON_GetObjectItem(j_tmp, "node");
	    cJSON *node = node_list->child;
	    for (; node; node = node->next) {
		cJSON *one_attr = node->child;
		char *name = one_attr->valuestring;
		char *state = one_attr->next->valuestring;
		char *ip_port = one_attr->next->next->valuestring;
		if (!name || !state || !ip_port || (pz->create(pz, name, ip_port, state) < 0)) {
		    fprintf(stderr, "[%s]->create node error\n", node->string);
		    return -1;
		}
	    }
	}
    }
    return 0;
}

static CIZone * m_create_zone(const char *name)
{
    CIManager * pm = &g_manager;
    CIZone *pz = pm->z_head;
    for (; pz; pz=pz->next) {
	if (strcmp(pz->name, name) == 0 ) { return NULL; }
    }
    pz = (CIZone *)malloc(sizeof(CIZone));
    if (pz == NULL) { return NULL; }
    GIz_init(pz, name);

    pz->prev = pm->z_tail;
    pz->next = NULL;
    if (pm->z_tail) { pm->z_tail->next = pz; }
    else { pm->z_head = pz; }
    pm->z_tail = pz;

    return pz;
}

static int m_create_ns(const char *name)
{
    CIManager * pm = &g_manager;
    CINameSpace *pns = pm->ns_head;
    for (; pns; pns = pns->next) {
	if (strcmp(pns->name, name) == 0) { return -1; }
    }

    char db_args[_DFS_MAX_LEN] = {0};
    snprintf(db_args, sizeof(db_args), "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
	    MNGR_DATA_DIR, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);

    pns = malloc(sizeof(CINameSpace));
    if (pns == NULL || GIns_init(pns, name, db_args, pm->primary) < 0) { 
	if (pns) { 
	    free(pns); 
	    pns = NULL;
	}
	return -1;
    }

    /*
    strncpy(pns->name, name, sizeof(pns->name));
    char indexdb_path[_DFS_MAX_LEN] = {0};

    snprintf(indexdb_path, sizeof(indexdb_path), "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
	    MNGR_DATA_DIR, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);

    pns->index_db = kcdbnew();
    if (kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE|KCOTRYLOCK) == 0) {
	free(pns);
	return -1;
    }
    */

    pns->prev = pm->ns_tail;
    pns->next = NULL;
    if (pm->ns_tail) { pm->ns_tail->next = pns; }
    else { pm->ns_head = pns; }
    pm->ns_tail = pns;

    return 0;
}

static CINameSpace * m_get_ns(const char *ns)
{
    for (CINameSpace *pns = g_manager.ns_head; pns; pns = pns->next) {
	if (strcmp(pns->name, ns) == 0) { return pns; }
    }
    return NULL;
}

static CINode * m_get_node(const char *node_name, size_t len)
{
    char *p = malloc(len+1);
    if (p == NULL) { return NULL; }
    memset(p, 0, len+1);
    strncpy(p, node_name, len);

    for (CIZone *pz = g_manager.z_head; pz; pz=pz->next) {
	for (CINode *pn = pz->head; pn; pn=pn->next) {
	    if (strcmp(pn->name, p) == 0) { free(p); return pn; }
	}
    }
    free(p);
    return NULL;
}

static CINode * m_choose(const char *record)
{
    struct pn_list{
	CINode *ptr;
	struct pn_list *prev;
	struct pn_list *next;
    }*head = NULL, *tail = NULL;

    int err = 0;
    int num = 0;
    for (CIZone *pz = g_manager.z_head; pz; pz=pz->next) {
	char *p = strstr(record, pz->name);
	if (p == NULL) { continue; }

	char node[_DFS_NODENAME_LEN] = {0};
	char *p_sharp = strstr(p, "#");
	char *p_split = strstr(p, "|");
	if (p_split) { strncpy(node, p_sharp+1, p_split-p_sharp-1); }
	else { strcpy(node, p_sharp+1); }

	CINode *node_tmp = m_get_node(node, strlen(node));
	if ( node_tmp == NULL ) { continue; }
	if ((node_tmp->state == S_READ_WRITE) || (node_tmp->state == S_READ_ONLY)) { 
	    struct pn_list *p= malloc(sizeof(struct pn_list));
	    if (p == NULL) { err = 1; goto err1; }
	    memset(p, 0, sizeof(struct pn_list));

	    p->ptr = node_tmp;
	    p->prev = tail;
	    p->next = NULL;
	    if (head == NULL) { head = p; }
	    else { tail->next = p; }
	    tail = p;

	    num++;
	}
    }

    struct pn_list *tmp = head;

    if (num == 0) { err = 1; goto err1; }
    int order = rand() % num;
    for (int i=0; i<order; ++i) {
	tmp = tmp->next;
    }
    CINode *pn = tmp->ptr;

err1:
    while (head) {
	tmp = head;
	head = head->next;
	free(tmp);
    }
    if (err) { return NULL; }
    else { return pn; }
}

static char * m_get_history(const char *line, int order)
{
    int len = strlen(line);
    char *record = NULL;
    int count = 0, size;
    int pos_cur=len-1, pos_tail=len-1, pos_head=0;
    char last = '\0';

    for (pos_cur=len-1; pos_cur>=0; last = line[pos_cur--]) {
	if (line[pos_cur] == '$') {
	    if (last != '$') {
		pos_head = pos_cur+1;
		if (count == order) { break; }
	    }
	}
	else {
	    if (last == '$') {
		pos_tail = pos_cur;
		count++ ;
	    }
	    if (pos_cur == 0) { pos_head = pos_cur;}
	}
    }

    if (count < order) { return NULL; }
    size = pos_tail - pos_head + 1;
    record = malloc(size+1);
    if (record == NULL) { return NULL; }
    memset(record, 0, size+1);
    strncpy(record, &(line[pos_head]), size);
    if (strlen(record) == 0) { return NULL; }
    return record;
}

