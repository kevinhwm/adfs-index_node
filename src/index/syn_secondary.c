/* syn_secondary.c
 *
 * kevinhwm@gmail.com
 */

#include "manager.h"
#include "../md5.h"

int GIss_init()
{
    /*
     * 检查增量目录中team文件是否存在，
     *     如果不存在，退出	（缺少主Index）
     *     如果存在，读取TID_r，
     *     	检查本地TID_l文件是否存在
     *     	如果TID_l存在，比较两个TID	（重启）
     *     		如果不同，退出。
     *     	如果TID_l不存在，复制TID_r到本地为TID_l	（新建）
     *
     */

    char r_tid[64] = {0};
    char l_tid[64] = {0};

    extern CIManager g_manager;
    char r_tid_path[128] = {0};
    snprintf(r_tid_path, sizeof(r_tid_path), "%s/" MNGR_TEAM_R_F, g_manager.syn_dir);
    FILE *f_r_tid = fopen(r_tid_path, "r");
    if (f_r_tid == NULL) { return -1; }
    fread(r_tid, 1, sizeof(r_tid), f_r_tid);
    fclose(f_r_tid);

    FILE *f_l_tid = fopen(MNGR_TEAM_L_F, "r");
    if (f_l_tid != NULL) {
	fread(l_tid, 1, sizeof(l_tid), f_l_tid);
	fclose(f_l_tid);

	if (strcmp(r_tid, l_tid)) { return -1; }
    }
    else {
	f_l_tid = fopen(MNGR_TEAM_L_F, "w");
	fwrite(r_tid, 1, strlen(r_tid), f_l_tid);
	fclose(f_l_tid);
    }

    return 0;
}

int GIss_release()
{
    return 0;
}
