/*
 * 查看版本号
 * 不存在
 * 	v3.0.0
 * 存在
 * 	读取版本号
 *
 * while （不是当前版本）
 * 	读版本号
 * 	执行一步升级
 *
 */


#include <stdio.h>
#include "../adfs.h"


int get_curr_data_version()
{
    char version[256] = {0};

    FILE *fver = fopen("version", "r+");
    if (fver != NULL) {
	int len = fread(version, 1, sizeof(version), f);
	if (len >= sizeof(version)) { 
	    // information is too long 
	    fclose(fver);
	    return -1;
	}
	fclose(fver);
    }
    else { sprintf(version, "D0300"); }

    if (strcmp(version, "D0300") = 0) { update_0300_to_0301(); }
    else if (strcmp(version, "D0301") = 0) { return 0; }
    else { printf("Can not recognize this data version. Maybe it is newer."); return -1;}

    return 0;
}

static int update()
{
    return 0;
}

static int update_03020400_to_03030000()
{
    mkdir();
    rename();
    return 0;
}

