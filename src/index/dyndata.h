/* dyndata.h - dynamic data
 *
 * kevinhwm@gmail.com
 */

#ifndef __DYNAMIC_H__
#define __DYNAMIC_H__


struct {
    char *ptr;
}CIInfo;


struct {
    struct CILine *head;
    struct CILine *tail;
}CIBuffer;


struct {
    struct CIInfo *info;
    struct CIBuffer *buffer;
}CIDynData;


int GId_init(struct CIDynData *_this);

#endif // __DYNAMIC_H__

