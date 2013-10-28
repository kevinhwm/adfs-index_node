
#include <stdio.h>
#include "../adfs.h"


int get_curr_data_version()
{
    char version[256] = {0};

    FILE *fver = fopen("version", "r+");
    if (fver != NULL) {
	int len = fread(version, 1, sizeof(version), f);
	fclose(fver);
	if (len >= sizeof(version)) { return -1; }
    }
    else { sprintf(version, "D0300"); }

    if (strcmp(version, "D0300") = 0) { update_0300_to_0301(); }
    else if (strcmp(version, "D0301") = 0) { return 0; }
    else { printf("Can not recognize this data version. Maybe it is newer."); return -1;}
}

static int update()
{
    return 0;
}

static int update_0300_to_0301()
{
    return 0;
}


static int create_log_dir(const char *path)
{
    int res = opendir("log");
    if (res < 0) {
	if (mkdir("log", 0755) < 0) { return -1; }
    }
    else {
	closedir();
    }
    return 0;
}

static int create_data_dir(const char *path)
{
    int res = opendir("log");
    if (res < 0) {
	if (mkdir("log", 0755) < 0) { return -1; }
    }
    else {
	closedir();
    }
    return 0;
}
