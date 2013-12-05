/* syn_primary.c
 *
 * kevinhwm@gmail.com
 */

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "manager.h"
#include "../md5.h"

#define _DFS_INC_ID_MAX		16


static void *th_fun(void *param);
static int open_file(const char *syn_dir, const char *name_space);
static int close_file();
static int scan_fin(const char * dir);
static int get_fileid(char * name);


struct CISynPrim { 
    pthread_mutex_t lock; 
    pthread_t th_cls; 
    FILE *f_inc; 
    char f_name[512]; 
    int num;
} syn_prim = { }; 


int GIsp_init()
{
    /*
     * 读取本地TID_l，
     * 	   如果不存在，标记
     * 	   如果存在，标记，读取TID_l
     * 检查增量目录中team文件是否存在，
     *     如果存在，标记，读取TID_r
     * 	   如果不存在，标记
     * 比较
     * 	   如果（TID_l，TID_r有一个不存在）退出
     *     如果（TID_l，TID_r都存在）
     *     	如果不同，退出
     *     如果（TID_l，TID_r都不存在），复制本地TID_l到远程TID_r
     *
     */

    int l_exist = 0;
    char l_tid[64] = {0};
    int r_exist = 0;
    char r_tid[64] = {0};

    FILE *f_l_tid = fopen(MNGR_TEAM_L_F, "r");
    if (f_l_tid != NULL) {
	fread(l_tid, 1, sizeof(l_tid), f_l_tid);
	fclose(f_l_tid);
	l_exist = 1;
    }

    extern CIManager g_manager;
    char r_tid_path[128] = {0};
    snprintf(r_tid_path, sizeof(r_tid_path), "%s/" MNGR_TEAM_R_F, g_manager.syn_dir);
    FILE *f_r_tid = fopen(r_tid_path, "r");
    if (f_r_tid != NULL) {
	fread(r_tid, 1, sizeof(r_tid), f_r_tid);
	fclose(f_r_tid);
	r_exist = 1;
    }

    if (l_exist ^ r_exist) { return -1; }
    else if (l_exist && r_exist) {
	if (strcmp(l_tid, r_tid)) { return -1; }
    }
    else { 
	unsigned char buf[16] = {0};
	unsigned char src[128] = {0};
	snprintf((char *)src, sizeof(src)-1, "%u%u", rand(), (unsigned int)time(NULL));
	md5(src, strlen((char *)src), buf);
	for (int i=0; i<16; ++i) {
	    sprintf(l_tid+2*i, "%02x", buf[i]);
	}
	f_l_tid = fopen(MNGR_TEAM_L_F, "a");
	if (f_l_tid == NULL) { return -1; }
	fwrite(l_tid, 1, strlen(l_tid), f_l_tid);
	fclose(f_l_tid);

	f_r_tid = fopen(r_tid_path, "a");
	if (f_r_tid == NULL) { return -1; }
	fwrite(l_tid, 1, strlen(l_tid), f_r_tid);
	fclose(f_r_tid);
    }

    memset(&syn_prim, 0, sizeof(syn_prim));
    if (pthread_mutex_init(&syn_prim.lock, NULL) != 0) { return -1; }
    if (pthread_create(&syn_prim.th_cls, NULL, th_fun, NULL) < 0) { return -1; }

    return 0;
}

int GIsp_release()
{
    pthread_cancel(syn_prim.th_cls);

    pthread_mutex_lock(&syn_prim.lock);
    close_file();
    pthread_mutex_unlock(&syn_prim.lock);

    pthread_mutex_destroy(&syn_prim.lock);
    return 0;
}

int GIsp_export(const char *syn_dir, const char *name_space, const char *key, const char *val)
{
    pthread_mutex_lock(&syn_prim.lock);

    open_file(syn_dir, name_space);
    fprintf(syn_prim.f_inc, "%s\t%s\n", key, val);
    fflush(syn_prim.f_inc);

    pthread_mutex_unlock(&syn_prim.lock);
    return 0;
}

static void *th_fun(void *param)
{
    sleep(60*60);
    pthread_mutex_lock(&syn_prim.lock);
    close_file();
    pthread_mutex_unlock(&syn_prim.lock);
    return 0;
}

static int open_file(const char *syn_dir, const char *name_space)
{
    char buf[512] = {0};
    char fname[512] = {0};
    snprintf(buf, sizeof(buf), "%s/%s", syn_dir, name_space);
    if (create_dir(buf) < 0) { return -1; }
    syn_prim.num = scan_fin(buf) +1 ;
    snprintf(fname, sizeof(fname), "%s/%d", buf, syn_prim.num);

    if (syn_prim.f_inc == NULL) {
	syn_prim.f_inc = fopen(fname, "a");
	if (syn_prim.f_inc == NULL) { return -1; }
	strncpy(syn_prim.f_name, fname, sizeof(syn_prim.f_name));
    }
    else if (strcmp(syn_prim.f_name, fname)){
	close_file();

	syn_prim.f_inc = fopen(fname, "a");
	if (syn_prim.f_inc == NULL) { return -1; }
	strncpy(syn_prim.f_name, fname, sizeof(syn_prim.f_name));
    }
    return 0;
}

static int close_file()
{
    if (syn_prim.f_inc) {
	fclose(syn_prim.f_inc);
	syn_prim.f_inc = NULL;
	char buf[512] = {0};
	sprintf(buf, "%s.fin", syn_prim.f_name);
	rename(syn_prim.f_name, buf);
	memset(syn_prim.f_name, 0, sizeof(syn_prim.f_name));
	syn_prim.num++;
    }
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

