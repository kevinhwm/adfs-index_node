/* conf.c
 *
 * kevinhwm@gmail.com
 */

#include <stdio.h>
#include <stdlib.h>
#include "def.h"

static void conf_read_and_filter(const char *conf_file, char **data);

cJSON * conf_parse(const char *conf_file)
{
    char *data = NULL;
    cJSON *json = NULL;
    conf_read_and_filter(conf_file, &data);
    json = cJSON_Parse(data);
    if (data) { free(data); }
    return json;
}

void conf_release(cJSON *json) { cJSON_Delete(json); }

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

