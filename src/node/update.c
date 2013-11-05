/* update.c
 *
 * kevinhwm@gmail.com
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>	// mkdir
#include <unistd.h>
#include <dirent.h>
#include "../adfs.h"
#include "manager.h"

static int identify(const char *version);
static int update(const char *version);
static int update_N0300();
static int create_dir(const char *path);

extern ANManager g_manager;
static char ADFS_DATA_NODE[][16] = {"N0300", "N0301"};

int anu_init()
{
    const char *ver_file = "version";
    char version[256] = {0};

    FILE *fver = fopen(ver_file, "r");
    if (fver != NULL) {
	int len = fread(version, 1, sizeof(version), fver);
	fclose(fver);
	if (len >= sizeof(version)) { return -1; }
    }
    else { sprintf(version, "%s", ADFS_DATA_NODE[0]); }

    if (identify(version) < 0) { return -1; }
    if (update(version) < 0) { return -1; }

    return 0;
}

static int identify(const char *version)
{
    size_t all_len = sizeof(ADFS_DATA_NODE);
    size_t len = sizeof(ADFS_DATA_NODE[0]);
    int num = all_len/len;

    for (int i=0; i<num; ++i) {
	if (strcmp(version, ADFS_DATA_NODE[i]) == 0) { return 0; }
    }
    return -1;
}

static int update(const char *version)
{
    if (update_N0300() < 0) { return -1; }
    return 0;
}

static int update_N0300()
{
    ANManager *pm = &g_manager;
    if (create_dir(pm->data_dir) < 0) { return -1; }
    if (create_dir(pm->log_dir) < 0) { return -1; }

    DIR *dp = NULL;
    struct dirent *dirp = NULL;

    dp = opendir(".");
    if (dp == NULL) { return -1; }
    while ((dirp = readdir(dp)) != NULL) {
	if (dirp->d_type == DT_DIR) {
	    if ((dirp->d_name[0] != '.') && (strcmp(dirp->d_name, pm->data_dir) != 0) && (strcmp(dirp->d_name, pm->log_dir) != 0)) { 
		char tmp[512] = {0};
		sprintf(tmp, "%s/%s", pm->data_dir, dirp->d_name);
		if (rename(dirp->d_name, tmp) < 0) { return -1; }
	    }
	}
	else if (strstr(dirp->d_name, ".log") != NULL) {
	    char tmp[512] = {0};
	    sprintf(tmp, "%s/%s", pm->log_dir, dirp->d_name);
	    if (rename(dirp->d_name, tmp) < 0) { return -1; }
	}
    }
    closedir(dp);
    FILE *f = fopen("version", "w");
    fprintf(f, "N0301");
    fclose(f);
    return 0;
}

static int create_dir(const char *path)
{
    DIR *dp = NULL;
    dp = opendir(path);
    if (dp == NULL) {
	if (mkdir(path, 0755) < 0) { return -1; }
    }
    else { closedir(dp); }
    return 0;
}

