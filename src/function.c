/* ai_function.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pcre.h>
#include "cJSON.h"

static void conf_read_and_filter(const char *conf_file, char **data);
static int match(char *src, char *pattern);

cJSON * conf_parse(char *conf_file)
{
    char *data = NULL;
    cJSON *json = NULL;

    conf_read_and_filter(conf_file, &data);
    json = cJSON_Parse(data);
    if (data) {free(data);}
    return json;
}

void conf_release(cJSON *json) { cJSON_Delete(json); }

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

static void conf_read_and_filter(const char *conf_file, char **data)
{
    long flen;
    char buf[1024];
    char *pline, *pd;
    FILE *f;

    f = fopen(conf_file, "rb");
    fseek(f, 0, SEEK_END);
    flen = ftell(f);
    fseek(f, 0, SEEK_SET);
    *data = malloc(flen+1);	// caller need to free this 
    pd = *data;
    while ( (pline = fgets(buf, sizeof(buf), f)) ) {
	while (*pline > 0x00 && *pline <= 0x20) {++pline;}
	if (*pline == '#' || *pline == 0x00) {continue;}
	strcpy(pd, buf);
	pd += strlen(buf);
    }
    return ;
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

    if (re == NULL) {
	//printf("PCRE compilation failed at offset %d: %s/n", erroffset, error);
	return -1;
    }

    rc = pcre_exec(re, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);

    if (rc < 0) {
	//if (rc == PCRE_ERROR_NOMATCH) printf("Sorry, no match ...\n");
	//else printf("Matching error %d\n", rc);
	pcre_free(re);
	return -1;
    }

    /*
       printf("\nOK, has matched ...\n\n");
       for (i = 0; i < rc; i++) {
       char *substring_start = src + ovector[2*i];
       int substring_length = ovector[2*i+1] - ovector[2*i];
       printf("$%2d: %.*s\n", i, substring_length, substring_start);
       }
       */

    pcre_free(re);
    return 0;
}

