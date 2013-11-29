/* function.c
 *
 * kevinhwm@gmail.com
 */

#include <string.h>
#include <pcre.h>
#include <sys/stat.h>	// mkdir
#include <dirent.h>

static int match(char *src, char *pattern);

int create_dir(const char *path)
{
    DIR *dp = NULL;
    dp = opendir(path);
    if (dp == NULL) {
	if (mkdir(path, 0755) < 0) { return -1; }
    }
    else { closedir(dp); }
    return 0;
}

int get_filename_from_url(char * p, char *pattern)
{
    if (p == NULL) {return -1;}
    char *pos = strstr(p, "?");
    if (pos != NULL) {pos[0] = '\0';}
    if (match(p, pattern) != 0) {return -1;}

    size_t len = 0;
    len = strlen(p);
    if (len == 0) {return -1;}

    while (p[len-1] == '/') {
	p[len-1] = '\0';
	len = strlen(p);
	if (len == 0) {return -1;}
    }

    while (p[0] == '/') {
	for (size_t i=1; i<=len; ++i) {p[i-1] = p[i];}
	len = strlen(p);
	if (len == 0) {return -1;}
    }
    return 0;
}

static int match(char *src, char *pattern)
{
    pcre *re;
    int erroffset;
    const char *error;
    const int OVECCOUNT = 16;
    int ovector[OVECCOUNT];
    int rc;

    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) { return -1; }

    rc = pcre_exec(re, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);
    if (rc < 0) { pcre_free(re); return -1; }

    pcre_free(re);
    return 0;
}

