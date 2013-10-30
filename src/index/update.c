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

char ADFS_DATA_INDEX[][16] = {"I0300", "I0301"};

static int identify(const char *version);
static int update_0300_to_0301();
static int create_dir(const char *path);

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
    else { 
	FILE *fnew = fopen(ver_file, "w");
	if (fnew == NULL) {return -1;}
	sprintf(version, ADFS_DATA_INDEX[0]); 
	fprintf(fnew, "%s", version);
	fclose(fnew);
    }

    if (identify(version) < 0) { printf("Can not recognize this data version. Maybe it is newer.\n"); return -1;}
    if (update(version) < 0) { printf("Update error.\n"); return -1;}

    return 0;
}

static int identify(const char *version)
{
    size_t all_len = sizeof(ADFS_DATA_INDEX);
    size_t len = sizeof(ADFS_DATA_INDEX[0]);
    int num = all_len/len;

    for (int i=0; i<num; ++i) {
	printf("%s\n", ADFS_DATA_INDEX[i]);
	if (strcmp(version, ADFS_DATA_INDEX[i]) == 0) { return 0; }
    }
    return -1;
}

static int update_0300_to_0301()
{
    return 0;
}

static int create_dir(const char *path)
{
    DIR *dirp = NULL;
    dirp = opendir(path);
    if (dirp == NULL) {
	if (mkdir(path, 0755) < 0) { return -1; }
    }
    else { closedir(dirp); }
    return 0;
}

