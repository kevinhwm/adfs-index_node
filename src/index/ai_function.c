/* conf.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/cJSON.h"
//#include <ctype.h>
//#include "adfs.h"


static void conf_read_and_filter(const char *conf_file, char **data);

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

static void conf_read_and_filter(const char *conf_file, char **data)
{
    long flen;
    char buf[1024];
    char *pb, *pd;
    FILE *f;

    f = fopen(conf_file, "rb");
    fseek(f, 0, SEEK_END);
    flen = ftell(f);
    fseek(f, 0, SEEK_SET);
    *data = malloc(flen+1);
    pd = *data;
    while ( (pb = fgets(buf, sizeof(buf), f)) ) {
	while (*pb > 0x00 && *pb <= 0x20) {++pb;}
	if (*pb == '#' || *pb == 0x00) {continue;}
	strcpy(pd, buf);
	pd += strlen(buf);
    }
    return ;
}

int get_filename_from_url(char * p)
{
    if (p == NULL) {return -1;}
    char *pos = strstr(p, "?");
    if (pos != NULL) {pos[0] = '\0';}

    int len = 0;
    len = strlen(p);
    if (len == 0) {return -1;}
    if (p[len-1] == '/') {p[len-1] = '\0';}
    len = strlen(p);
    if (len == 0) {return -1;}

    if (p[0] == '/') {
        for (int i=1; i<=len; ++i) {p[i-1] = p[i];}
    }
    if (strlen(p) <= 0) {return -1;}

    return 0;
}

