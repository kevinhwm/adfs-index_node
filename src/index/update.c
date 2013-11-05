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
static int update_I0300();
static int create_dir(const char *path);

extern AIManager g_manager;
static char ADFS_DATA_INDEX[][16] = {"I0300", "I0301"};

int aiu_init()
{
    const char *ver_file = "version";
    char version[256] = {0};

    FILE *fver = fopen(ver_file, "r");
    if (fver != NULL) {
	int len = fread(version, 1, sizeof(version), fver);
	fclose(fver);
	if (len >= sizeof(version)) { return -1; }
    }
    else { sprintf(version, "%s", ADFS_DATA_INDEX[0]); }

    if (identify(version) < 0) { return -1; }
    if (update(version) < 0) { return -1; }

    return 0;
}

static int identify(const char *version)
{
    size_t all_len = sizeof(ADFS_DATA_INDEX);
    size_t len = sizeof(ADFS_DATA_INDEX[0]);
    int num = all_len/len;

    for (int i=0; i<num; ++i) {
	if (strcmp(version, ADFS_DATA_INDEX[i]) == 0) { return 0; }
    }
    return -1;
}

static int update(const char *version)
{
    if (update_I0300() < 0) { return -1; }
    return 0;
}

static int update_I0300()
{
    AIManager *pm = &g_manager;
    if (create_dir(pm->data_dir) < 0) { return -1; }
    if (create_dir(pm->log_dir) < 0) { return -1; }

    DIR *dp = NULL;
    struct dirent *dirp = NULL;

    dp = opendir(".");
    if (dp == NULL) { return -1; }
    while ((dirp = readdir(dp)) != NULL) {
	if (dirp->d_type == DT_DIR) { continue; }
	if (strstr(dirp->d_name, ".kch") != NULL) {
	    char tmp[512] = {0};
	    sprintf(tmp, "%s/%s", pm->data_dir, dirp->d_name);
	    if (rename(dirp->d_name, tmp) < 0) { return -1; }
	}
	if (strstr(dirp->d_name, ".log") != NULL) {
	    char tmp[512] = {0};
	    sprintf(tmp, "%s/%s", pm->log_dir, dirp->d_name);
	    if (rename(dirp->d_name, tmp) < 0) { return -1; }
	}
    }
    closedir(dp);
    FILE *f = fopen("version", "w");
    fprintf(f, "I0301");
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

