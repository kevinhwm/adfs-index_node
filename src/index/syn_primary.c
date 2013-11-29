/* syn_primary.c
 *
 * kevinhwm@gmail.com
 */

#include "manager.h"
#include "../md5.h"

static struct CISynPrim{
    pthread_mutex_t lock;
    FILE *f_inc
    char f_name[512];
    int count;
}syn_prim;

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

    memset(syn_prim, 0, sizeof(syn_prim));
    if (pthread_mutex_lock(&syn_prim.lock, NULL) != 0) { return -1; }

    return 0;
}

int GIsp_release()
{
    pthread_mutex_lock(&syn_prim.lock);
    if (syn_prim.f_inc) { fclose(syn_prim.f_inc); }
    pthread_mutex_unlock(&syn_prim.lock);
    pthread_mutex_destroy(&syn_prim.lock);
    return 0;
}

int GIsp_export(const char *syn_dir, const char *name_space, const char *key, const char *val)
{

    char buf[512] = {0};
    char fname[512] = {0};
    snprintf(buf, sizeof(buf), "%s/%s", syn_dir, name_space);
    if (create_dir(buf) < 0) { return -1; }
    snprintf(fname, sizeof(fname), "%s/%s", buf, count);

    if (f_inc == NULL) {
	FILE *f_inc = fopen(fname, "a");
	if (f_inc == NULL) { return -1; }
	strncpy(f_name, fname, sizeof(f_name));
    }
    else if (strcmp(f_name, fname)){
	fclose(f_inc);
	f_inc = fopen(fname, "a");
	if (f_inc == NULL) { return -1; }
	strncpy(f_name, fname, sizeof(f_name));
    }

    fprintf(f_inc, "%s\t%s", key, val);

    return 0;
}

