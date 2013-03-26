
#include <dirent.h>
#include <string.h>
#include "an.h"


KCDB * index_db = NULL;

unsigned long kc_apow = 0;              // sets the power of the alignment of record size
unsigned long kc_fbp  = 10;             // sets the power of the capacity of the free block pool
unsigned long kc_bnum = 1000000;        // sets the number of buckets of the hash table
unsigned long kc_msiz = 32;             // sets the size of the internal memory-mapped region

NodeDBList ndb_list;


int check_name(char * name);
int count_kch(char * srcip);


int check_name(char * name)
{
    if (name == NULL)
        return ADFS_ERROR;

    if (strlen(direntp->d_name) > 8)
        return ADFS_ERROR;

    char *pos = strstr(direntp->d_name, ".kch");
    if (pos == NULL)
        return ADFS_ERROR;

    for (int i=0; i < pos-name, i++)
    {
        if (name[i] < '0' || name[i] > '9')
            return ADFS_ERROR;
    }

    if (strcmp(pos, ".kch") != 0)
        return ADFS_ERROR;

    return ADFS_OK;
}


int count_kch(char * srcip)//count how many  kchfile in a folder
{
    int total_count=0;

    DIR* dirp;
    struct dirent* direntp;

    dirp = opendir( srcip.c_str() );
    if( dirp != NULL ) 
    {
        while ((direntp = readdir(dirp)) != NULL)
        {
            if (check_name(direntp->d_name) == ADFS_OK)
                total_count++;
        }

        closedir( dirp );
        if (total_count>0)
            return total_count;
        else
            return 1;
    }
    return 0;
}



void an_init(char *dbpath, unsigned long cache_size, unsigned long kchfile_size, unsigned long max_node_count)
{
    int node_num = count_kch(db_path);
    char * cache_mode = 'n';
    int init_mode = 1;

    kc_msiz = cache_size * 1024*1024;

    // ------------------ index db of ADFS-Node ------------------
    char path[1024] = {0};
    if (sprintf(path, "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                dbpath, kc_apow, kc_fbp, kc_bnum *40, kc_msiz*10) >= sizeof(path))
    {
        return ADFS_ERROR;
    }
    
    int32_t res = kcdbopen( index_db, path, KCOWRITER|KCOCREATE );
    if ( !res ) 
        return ADFS_ERROR;

    // ------------------ node db of ADFS-Node ------------------
    InitNodeDBList(&ndb_list);

    char node_path[1024] = {0};
    for (int i=1; i <= node_num; i++)
    {
        if (snprintf(node_path, sizeof(node_path), "%s/%d.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                    dbpath, i, kc_apow, kc_fbp, kc_bnum*3, kc_msiz) >= sizeof(node_path))
        {
            return ADFS_ERROR;
        }

        if (i < node_num)
            ndb_list.create(&ndb_list, i, node_path, strlen(node_path), S_READ_ONLY);
        else
            ndb_list.create(&ndb_list, i, node_path, strlen(node_path, S_READ_WRITE));
    }

    //get size. if too big, split


    return 0;
}


