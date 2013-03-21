
#include <adfsnode.h>


KCDB * index_db = NULL;

unsigned long index_apow = 0;               // sets the power of the alignment of record size
unsigned long index_fbp  = 10;              // sets the power of the capacity of the free block pool
unsigned long index_bnum = 1000000;         // sets the number of buckets of the hash table
unsigned long index_msiz = 32 * 1024*1024;  // sets the size of the internal memory-mapped region


void an_init(char *dbpath, unsigned long cache_size, unsigned long kchfile_size, unsigned long max_node_count)
{
    int node_num = count_kch(db_path);
    char * cache_mode = 'n';
    int init_mode = 1;

    index_msiz = cache_size;

    // ------------------ index db of ADFS-Node ------------------
    char path[1024] = {0};
    if (sprintf(path, "%s/indexdb.kch#apow=%lu#fpow=%lu#bnum=%lu#msiz=%lu", 
                dbpath, index_apow, index_fbp, index_bnum, index_msiz) >= sizeof(path))
    {
        return ADFS_ERROR;
    }
    
    int32_t res = kcdbopen( index_db, path, KCOWRITER|KCOCREATE );
    if ( !res ) 
        return ADFS_ERROR;

    // ------------------ node db of ADFS-Node ------------------
    NodeDBList ndb_list;
    InitNodeDBList(&ndb_list);

    char node_path[1024] = {0};
    for (int i=1; i <= node_num; i++)
    {
        if (snprintf(node_path, sizeof(node_path), "%s/%d.kch", dbpath, i) >= sizeof(node_path))
            return ADFS_ERROR;
        if (i < node_num)
            ndb_list.create(&ndb_list, i, node_path, strlen(node_path), S_READ_ONLY);
        else
            ndb_list.create(&ndb_list, i, node_path, strlen(node_path, S_READ_WRITE));
    }

    return 0;
}

