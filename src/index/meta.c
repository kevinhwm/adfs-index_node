/* meta.c
 *
 * kevinhwm@gmail.com
 */

#include <stdlib.h>	// rand
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>	// gettimeofday
#include "meta.h"


/*
static int l_release(struct CILine *_this);
static int l_add_rec(struct CILine *_this);
static int l_del_rec(struct CILine *_this);
static char * l_output(struct CILine *_this);


int GIL_init(struct CILine *_this)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CILine));

    _this->split = '$';
    _this->release = l_release();
    _this->add_rec = l_add_rec;
    _this->del_rec = l_del_rec;
    _this->output = l_output;
    return 0;
}


static int f_release(struct CIFile *_this);
static int f_add_pos(struct CIRcord *_this);
static int f_del_pos(struct CIRcord *_this);
static int f_output(struct CIRcord *_this);



int GIf_init(struct CIFile *_this)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CILine));

    _this->split = '|';
    _this->release = f_release;
    _this->add_pos = f_add_pos;
    _this->del_pos = f_del_pos;
    _this->output = f_output;
    return 0;
}
*/




static int f_release(CIFile *_this);
static int f_add(CIFile *_this, const char *zone, const char *node);
static char * f_get_string(CIFile *_this);

////////////////////////////////////////////////////////////////////////////////
// just function
static void create_uuid(CIFile *_this);

int GIf_init(CIFile *_this)
{
    if (_this == NULL) { return -1; }
    memset(_this, 0, sizeof(CIFile));

    _this->add = f_add;
    _this->release = f_release;
    _this->get_string = f_get_string;
    create_uuid(_this);
    return 0;
}

static int f_release(CIFile *_this)
{
    for (struct CIPosition *pp = _this->head, *tmp = NULL;
	    pp;
	)
    {
	tmp=pp; 
	pp=pp->next; 
	free(tmp); 
    }
    return 0;
}

static int f_add(CIFile *_this, const char *zone, const char *node)
{
    CIPosition *pp = malloc(sizeof(CIPosition));
    if (pp == NULL) { return -1; }
    _this->num++;
    snprintf(pp->zone_node, sizeof(pp->zone_node), "%s#%s", zone, node);

    pp->prev = _this->tail;
    pp->next = NULL;
    if (_this->tail) {_this->tail->next = pp;}
    else {_this->head = pp;}
    _this->tail = pp;
    return 0;
}

static char * f_get_string(CIFile *_this)
{
    int len = _DFS_UUID_LEN + _this->num * (sizeof(_this->head->zone_node)+1);
    char *a_file = malloc(len);
    if (a_file == NULL) {return NULL;}
    memset(a_file, 0, len);
    strcpy(a_file, _this->uuid);

    CIPosition *pp = _this->head;
    while (pp) {
	strcat(a_file, pp->zone_node);
	strcat(a_file, "|");
	pp = pp->next;
    }
    // set the last "|" to be "\0"
    a_file[strlen(a_file)-1] = '\0';
    return a_file;
}

////////////////////////////////////////////////////////////////////////////////
static void create_uuid(CIFile *_this)
{
    time_t t;
    struct tm lt;
    struct timeval tv;
    char buf[32] = {0};

    time(&t);
    localtime_r(&t, &lt);
    gettimeofday(&tv, NULL);
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &lt);
    snprintf(_this->uuid, sizeof(_this->uuid), "_%s%06ld%03d", buf, tv.tv_usec, rand()%1000);
}

