/* ai_manager.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <string.h>
#include <dirent.h>     // opendir
#include <sys/stat.h>   // mkdir
#include <kclangc.h>
#include <curl/curl.h>
#include <time.h>

#include "adfs.h"
#include "ai_manager.h"
#include "ai_record.h"


static ADFS_RESULT m_init_zone(const char *conf_file);
static ADFS_RESULT m_init_ns(const char *conf_file);
static ADFS_RESULT m_init_log(const char *conf_file);
static ADFS_RESULT m_init_stat(const char *conf_file);
static AIZone * m_create_zone(const char *name, int weight);
static ADFS_RESULT m_create_ns(const char *name);
static AINameSpace * m_get_ns(const char *ns);
static AIZone * m_choose_zone(const char * record);
static char * m_get_history(const char *, int);

AIManager g_manager;
unsigned long g_MaxFileSize;
LOG_LEVEL g_log_level;
int g_clean_mode = 0;

ADFS_RESULT aim_init(const char *conf_file, const char *path, unsigned long mem_size, unsigned long max_file_size)
{
    AIManager *pm = &g_manager;
    memset(pm, 0, sizeof(*pm));

    DIR *dirp = opendir(path);
    if( dirp == NULL )
        return ADFS_ERROR;
    closedir(dirp);

    strncpy(pm->db_path, path, sizeof(pm->db_path));
    pm->kc_apow = 0;
    pm->kc_fbp = 10;
    pm->kc_bnum = 1000000;
    pm->kc_msiz = mem_size *1024*1024;
    // init zone
    if (m_init_zone(conf_file) == ADFS_ERROR)
        return ADFS_ERROR;
    // init namespace
    if (m_init_ns(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;
    // init log
    if (m_init_log(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;
    // init statistics
    if (m_init_stat(conf_file) == ADFS_ERROR)
	return ADFS_ERROR;
    g_MaxFileSize = max_file_size *1024*1024;
    // init libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    // init rand
    srand(time(NULL));

    // work mode 
    char value[ADFS_FILENAME_LEN] = {0};
    if (conf_read(conf_file, "work_mode", value, sizeof(value)) == ADFS_ERROR)
	return ADFS_ERROR;
    if (strcmp(value, "clean") == 0) {
	printf("\nIndex will work in 'clean mode'! \n"
		"When it is done, you should restart it manually. \n");
	while (1) {
	    printf("\nAre you sure ? (y/n) ");
	    int i = getchar();
	    if (i == 'y') {
		g_clean_mode = 1;
		printf("\nwork in 'clean' node.\n");
		return ADFS_OK;
	    }
	    else if (i == 'n') {
		break;
	    }
	}
    }
    printf("\nwork in 'normal' node.\n");
    return ADFS_OK;
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

    pm->s_upload.release(&(pm->s_upload));
    pm->s_download.release(&(pm->s_download));
    pm->s_delete.release(&(pm->s_delete));

    log_release();
    curl_global_cleanup();
}

ADFS_RESULT aim_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len)
{
    log_out("upload", "upload", LOG_LEVEL_INFO);
    DBG_PRINTSN("aim upload 1");
    AIManager *pm = &g_manager;

    int exist = 1;
    char *old_list = NULL;
    size_t old_list_len;
    AINameSpace *pns = NULL;
    if (name_space)
	pns = m_get_ns(name_space);
    else
	pns = m_get_ns("default");

    if (pns == NULL) 
	return ADFS_ERROR;
    old_list = kcdbget(pns->index_db, fname, strlen(fname), &old_list_len);
    if (old_list == NULL || old_list[old_list_len-1] == '|')
        exist = 0;
    if (exist && !overwrite)		// exist and not overwrite
	goto ok1;
    // send file
    AIRecord air;
    air_init(&air);

    DBG_PRINTSN("aim upload 30");
    AIZone *pz = pm->z_head;
    while (pz) {
	AINode * pn = pz->rand_choose(pz);
	char url[ADFS_MAX_PATH] = {0};
	if (name_space)
	    sprintf(url, "http://%s/upload_file/%s%.*s?namespace=%s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid, name_space);
	else
	    sprintf(url, "http://%s/upload_file/%s%.*s", pn->ip_port, fname, ADFS_UUID_LEN, air.uuid);

	DBG_PRINTSN("aim upload 35");
	if (aic_upload(pn, url, fname, fdata, fdata_len) == ADFS_ERROR) {
	    printf("upload error: %s\n", url);
	    // failed. roll back.
	}
	air.add(&air, pz->name, pn->ip_port);
	pz = pz->next;
    }
    DBG_PRINTSN("aim upload 40");

    // add record
    char *record = air.get_string(&air);
    if (record == NULL)
	goto err1;

    DBG_PRINTSN("aim upload 45");
    if (old_list == NULL) {
	kcdbset(pns->index_db, fname, strlen(fname), record, strlen(record));
    }
    else {
	char *new_list = malloc(old_list_len + strlen(record) + 2);
	if (new_list == NULL) 
	    goto err2;
	sprintf(new_list, "%s$%s", old_list, record);
	kcdbset(pns->index_db, fname, strlen(fname), new_list, strlen(new_list));
	free(new_list);
    }
    DBG_PRINTSN("aim upload 50");
    pm->s_upload.inc(&(pm->s_upload));

    free(record);
    air.release(&air);
ok1:
    kcfree(old_list);
    return ADFS_OK;

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
    log_out("download", "download", LOG_LEVEL_INFO);
    //AIManager *pm = &g_manager;
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
	return NULL;

    DBG_PRINTSN(record);
    AIZone *pz = m_choose_zone(record + ADFS_UUID_LEN);
    if (pz == NULL)
	goto err1;

    char *pos_zone = strstr(record, pz->name);
    char *pos_sharp = strstr(pos_zone, "#");
    char *pos_split = strstr(pos_sharp, "|");
    if (pos_split)
	sprintf(url, "http://%.*s/download/%s%.*s", (int)(pos_split-pos_sharp-1), pos_sharp+1, fname, ADFS_UUID_LEN, record);
    else
	sprintf(url, "http://%s/download/%s%.*s", pos_sharp+1, fname, ADFS_UUID_LEN, record);
    strncat(url, "?namespace=", ADFS_MAX_PATH);
    strncat(url, pns->name, ADFS_MAX_PATH);
    DBG_PRINTSN(url);
    pm->s_download.inc(&(pm->s_download));

    free(record);
    kcfree(line);
    return url;
err1:
    free(record);
    kcfree(line);
    return NULL;
}

ADFS_RESULT aim_delete(const char *ns, const char *fname)
{
    log_out("delete", "delete", LOG_LEVEL_INFO);
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
    DBG_PRINTSN("10");
    while (pz) {
	DBG_PRINTSN("11");
	strncat(p, "<li><font color=\"blue\">Zone: ", size);
	strncat(p, pz->name, size);
	strncat(p, "</font></li><table border=\"1\">\n", size);
	strncat(p, "<tr><th>node</th><th>status</th></tr>\n", size);
	AINode *pn = pz->head;
	while (pn) {
	    DBG_PRINTSN("12");
	    strncat(p, "<tr><td>", size);
	    strncat(p, pn->ip_port, size);
	    strncat(p, "</td><td ", size);
	    char url[1024] = {0};
	    sprintf(url, "http://%s/status", pn->ip_port);
	    DBG_PRINTSN("13");
	    if (aic_status(pn, url) == ADFS_OK) 
		strncat(p, "bgcolor=\"green\"><font color=\"white\">alive</font>", size);
	    else 
		strncat(p, "bgcolor=\"red\"><font color=\"white\">lost</font>", size);
	    strncat(p, "</td></tr>", size);
	    pn = pn->next;
	    DBG_PRINTSN("14");
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
	sprintf(tmp, "%d", *pcount);
	strncat(p, tmp, size);
	strncat(p, "|", size);
	pcount--;
    }

    DBG_PRINTSN("20");
    return p;
}

// private
static ADFS_RESULT m_init_zone(const char *conf_file)
{
    char value[ADFS_FILENAME_LEN] = {0};
    // create zone
    if (conf_read(conf_file, "zone_num", value, sizeof(value)) == ADFS_ERROR)
        return ADFS_ERROR;
    int zone_num = atoi(value);
    if (zone_num <= 0)
        return ADFS_ERROR;
    for (int i=0; i<zone_num; ++i)
    {
        char key[ADFS_FILENAME_LEN] = {0};
        char name[ADFS_FILENAME_LEN] = {0};
        char weight[ADFS_FILENAME_LEN] = {0};
        char num[ADFS_FILENAME_LEN] = {0};
        snprintf(key, sizeof(key), "zone%d_name", i);
        if (conf_read(conf_file, key, name, sizeof(name)) == ADFS_ERROR)
            return ADFS_ERROR;
        snprintf(key, sizeof(key), "zone%d_weight", i);
        if (conf_read(conf_file, key, weight, sizeof(weight)) == ADFS_ERROR)
            return ADFS_ERROR;
	AIZone *pz = m_create_zone(name, atoi(weight));
	if (pz == NULL)
	    return ADFS_ERROR;
        snprintf(key, sizeof(key), "zone%d_num", i);
        if (conf_read(conf_file, key, num, sizeof(num)) == ADFS_ERROR)
            return ADFS_ERROR;
        int node_num = atoi(num);
        if (node_num <= 0)
            return ADFS_ERROR;
        for (int j=0; j<node_num; ++j) {
            snprintf(key, sizeof(key), "zone%d_%d", i, j);
            if (conf_read(conf_file, key, value, sizeof(value)) == ADFS_ERROR)
                return ADFS_ERROR;
            if (strlen(value) <= 0)
                return ADFS_ERROR;
	    // create node
            if (pz->create(pz, value) == ADFS_ERROR) 
                return ADFS_ERROR;
        }
    }
    return ADFS_OK;
}

static ADFS_RESULT m_init_ns(const char *conf_file)
{
    char value[ADFS_FILENAME_LEN] = {0};
    // create namespace
    if (conf_read(conf_file, "namespace_num", value, sizeof(value)) == ADFS_ERROR)
        return ADFS_ERROR;
    int ns_num = atoi(value);
    if (ns_num <= 0 || ns_num >100)
        return ADFS_ERROR;
    if (m_create_ns("default") == ADFS_ERROR)
	return ADFS_ERROR;
    for (int i=0; i<ns_num; ++i)
    {
        char key[ADFS_FILENAME_LEN] = {0};
	sprintf(key, "namespace_%d", i);
	if (conf_read(conf_file, key, value, sizeof(value)) == ADFS_ERROR)
	    return ADFS_ERROR;
	if (m_create_ns(value) == ADFS_ERROR)
	    return ADFS_ERROR;
    }
    return ADFS_OK;
}

static ADFS_RESULT m_init_log(const char *conf_file)
{
    char value[ADFS_FILENAME_LEN] = {0};
    // log_level
    if (conf_read(conf_file, "log_level", value, sizeof(value)) == ADFS_ERROR)
        return ADFS_ERROR;
    g_log_level = atoi(value);
    if (g_log_level < 0 || g_log_level > 4)
	return ADFS_ERROR;

    if (conf_read(conf_file, "log_file", value, sizeof(value)) == ADFS_ERROR)
	return ADFS_ERROR;
    if (log_init(value) != 0)
	return ADFS_ERROR;
    return ADFS_OK;
}

static ADFS_RESULT m_init_stat(const char *conf_file)
{
    AIManager *pm = &g_manager;
    char value[ADFS_FILENAME_LEN] = {0};
    // statistics
    if (conf_read(conf_file, "stat_hour", value, sizeof(value)) == ADFS_ERROR)
	return ADFS_ERROR;
    int stat_hour = atoi(value);
    if (stat_hour < 1 || stat_hour > 24)
	return ADFS_ERROR;

    unsigned long stat_start = (unsigned long)time(NULL);
    int stat_min = stat_hour * 60;
    if (stat_init(&(pm->s_upload), stat_start, stat_min) == ADFS_ERROR)
	return ADFS_ERROR;
    if (stat_init(&(pm->s_download), stat_start, stat_min) == ADFS_ERROR)
	return ADFS_ERROR;
    if (stat_init(&(pm->s_delete), stat_start, stat_min) == ADFS_ERROR)
	return ADFS_ERROR;
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
    while (pns)
    {
	if (strcmp(pns->name, ns) == 0)
	    return pns;
	pns = pns->next;
    }
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
    pns = malloc(sizeof(AINameSpace));
    if (pns == NULL) 
	return ADFS_ERROR;

    strncpy(pns->name, name, sizeof(pns->name));
    char indexdb_path[ADFS_MAX_PATH] = {0};
    sprintf(indexdb_path, "%s/%s.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
            pm->db_path, name, pm->kc_apow, pm->kc_fbp, pm->kc_bnum, pm->kc_msiz);
    pns->index_db = kcdbnew();
    if (kcdbopen(pns->index_db, indexdb_path, KCOREADER|KCOWRITER|KCOCREATE) == 0) 
        return ADFS_ERROR;

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
    int count = 0;
    int pos=-1, pos1=len-1, pos2=-1;
    int size;
    for (pos=len-1; pos>=0; --pos) {
	if (line[pos] == '$') {
	    ++count;
	    if (count == order)
		pos1 = pos-1;
	    if (count == order+1)
		pos2 = pos+1;
	}
    }
    if (count < order)
	return NULL;
    else if (count == order)
	pos2 = 0;

    size = pos1 - pos2 + 1;
    record = malloc(size+1);
    if (record == NULL)
	return NULL;
    memset(record, 0, size);
    strncpy(record, &(line[pos2]), size);

    return record;
}


