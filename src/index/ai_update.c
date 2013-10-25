#include <stdio.h>

enum {
    VER_OLD      = 0x00000000,
    VER_03020400 = 0x03020400,
    VER_03030000 = 0x03030000
};

static unsigned long gs_version = VER_OLD;

int check_version()
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

