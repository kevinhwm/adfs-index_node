#include <stdio.h>
#include "curl_upload.h"

int main()
{
    curl_global_init(CURL_GLOBAL_ALL);
    char *fbuf= new char[512*1024];
    for( int i = 0; i< 1000; i++ )
    {
         char fname[64];
         sprintf( fname, "00000000001111111111222222222233.%08d", i );
         upload( "http://10.2.4.100:11911/upload_file", fname, fbuf, 512*1024, "md5.crc32" );

    }
    return 0;
}
