/* meta.h
 *
 * kevinhwm@gmail.com
 */

#ifndef __LINE_H__
#define __LINE_H__

#include "../def.h"


typedef struct CIPosition {
    char zone_node[_DFS_ZONENAME_LEN + _DFS_NODENAME_LEN + 2];

    struct CIPosition *prev;
    struct CIPosition *next;
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
}CIFile;


int GIf_init(CIFile *);

#endif // __LINE_H__

