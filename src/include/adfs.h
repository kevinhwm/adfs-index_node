/*
 * huangtao@antiy.com
 */

#define ADFS_VERSION        3.0


#define MAX_FILE_SIZE       0x08000000      // 2^3 * 16^6 = 2^27 = 128MB
#define SPLIT_FILE_SIZE     0x04000000      // 2^2 * 16^6 = 2^26 = 64MB


typedef enum _ADFS_RESULT
{
    ADFS_OK         = 0,
    ADFS_ERROR      = -1,
}ADFS_RESULT;


