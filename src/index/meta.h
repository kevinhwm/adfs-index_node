/* meta.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __LINE_H__
#define __LINE_H__

#include "../def.h"


typedef struct CIPosition {
    // <zone>#<node>
    /*
    char zone[_DFS_ZONENAME_LEN];
    char node[_DFS_NODENAME_LEN];
    char split;
    */
    char zone_node[_DFS_ZONENAME_LEN + _DFS_NODENAME_LEN + 2];

    struct CIPosition *prev;
    struct CIPosition *next;

    int (*init)(const char *zone, const char *node);
}CIPosition ;

typedef struct CIFile {
    // <uuid><position>|<position>|<position>
    char uuid[_DFS_UUID_LEN +8];
    int num;
    char split;

    struct CIPosition *head;
    struct CIPosition *tail;
    struct CIFile *prev;
    struct CIFile *next;

    int (*release)(struct CIFile *);
    int (*add)(struct CIFile *, const char *zone, const char *node);
    char * (*get_string)(struct CIFile *);

    /*
    int (*release)(struct CIFile *);
    int (*add_pos)(struct CIFile *, const char *zone, const char *node);
    int (*del_pos)(struct CIFile *, const char *zone, const char *node);
    char * (*output)(struct CIFile *);
    */
}CIFile;

/*
typedef struct {
    // <file>$<file>$<file>
    // <file>$<file>$<file>$
    int num;
    char split;

    struct CIFile *head;
    struct CIFile *tail;

    int (*release)(struct CILine *);
    int (*add_file)(struct CILine *);
    int (*del_file)(struct CILine *);
    char * (*output)(struct CILine *);
}CILine;

int GIl_init(struct CILine *);
*/

int GIf_init(CIFile *);

#endif // __LINE_H__

