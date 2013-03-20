
#include <adfsnode.h>

void an_init(char *db_path, long cache_size, long kchfile_size, int max_node_count)
{
    int node_num = count_kch(db_path);
    char * cache_mode = 'n';
    int init_mode = 1;

    char index_path[1024] = {0};
    sprintf(index_path, "%s/indexdb.kch", m_dbpath);
    return 0;
}
