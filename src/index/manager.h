/* 
 * manager.h
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#ifndef __MANAGER_H__
#define __MANAGER_H__

#include <kclangc.h>
#include "def.h"
#include "zone.h"


typedef struct AINameSpace
{
    char name[ADFS_NAMESPACE_LEN];

    // format: "uuidzone#node|zone#node$uuidzone#node|zone#node..."
    // zone#node					-> a position
    // zone#node|zone#node				-> multiple positions
    // uuidzone#node|zone#node				-> a record. length of uuid is exactly 24 bytes
    // uuidzone#node|zone#node$uuidzone#node|zone#node	-> that real records look like
    KCDB * index_db;

    struct AIRecord * head;
    struct AIRecord * tail;

    struct AINameSpace *pre;
    struct AINameSpace *next;
}AINameSpace;

typedef struct AIManager
{
    const char *msg;
    char db_path[ADFS_MAX_PATH];	// database file path

    unsigned long kc_apow;
    unsigned long kc_fbp;
    unsigned long kc_bnum;
    unsigned long kc_msiz;

    struct AINameSpace *ns_head;
    struct AINameSpace *ns_tail;

    struct AIZone *z_head;
    struct AIZone *z_tail;

}AIManager;


// init function
ADFS_RESULT mgr_init(const char *file_conf, const char *path_db, unsigned long mem_size);
// mgr_check();
// clean up function
void mgr_exit();

// recieve file function
ADFS_RESULT mgr_upload(const char *name_space, int overwrite, const char *fname, void *fdata, size_t fdata_len);
// send file function
char * mgr_download(const char *name_space, const char *fname);
// delete file function. it's not the real deletion
ADFS_RESULT mgr_delete(const char *name_space, const char *fname);

#endif // __MANAGER_H__
