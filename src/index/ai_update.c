/*
 * 查看版本号
 * 不存在
 * 	3.2.4.1
 * 存在
 * 	读取版本号
 *
 * while （不是当前版本）
 * 	读版本号
 * 	执行一步升级
 *
 */


#include <stdio.h>

enum {
    VER_OLD      = 0x00000000,
    VER_03020400 = 0x03020400,
    VER_03030000 = 0x03030000
};


static unsigned long gs_version = VER_OLD;

int get_version()
{
    FILE *fver = fopen("version", "r+");
    if (fver == NULL) {
	gs_version = VER_03020400;
    }
    else {
	char buf[1024] = {0};
	int len = fread(buf, 1, sizeof(buf), f);
	if (len > sizeof(buf)) { return -1; }
	fclose(fver);
    }
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

