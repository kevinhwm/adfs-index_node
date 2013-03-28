#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
//#include <fcntl.h>
#include <stdio.h>
#include <string.h>


extern KCDB* g_kcdb;

static nxweb_result delete_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("delete - request\n");

    nxweb_set_response_content_type(resp, "text/plain");

    // get the namespace. defaults using 
    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );

    char * url = malloc(strlen(req->path_info)+1);
    if (!url) 
        return NXWEB_ERROR;
    strcpy(url, req->path_info);
    url[strlen(req->path_info)] = '\0';

    nxweb_url_decode( url, 0 );
    char *fname = url, *temp;

    while( temp = strstr( fname, "/" ) )
        fname = temp + 1;
    char *strip_args = strstr( fname, "?" );
    int fname_len = strip_args?strip_args - fname:strlen( fname );
    char fname_with_space[1024]= {0};
    if( strlen( url ) > sizeof( fname_with_space )-1 ){
        char err_msg[1024] = {0};
        snprintf( err_msg, sizeof(err_msg)-1, "File not found<br/>namespace:%s<br/>filename:%.*s", name_space, fname_len, fname);
        nxweb_send_http_error( resp, 404, err_msg );
        free(url);
        return NXWEB_MISS;
    }

    if( name_space )
        sprintf( fname_with_space, "%s:/", name_space);
    else
        strcpy( fname_with_space, ":/" );
    strncat( fname_with_space, fname, fname_len );

    int32_t succ = kcdbremove( g_kcdb, fname_with_space, strlen( fname_with_space) );
    nxweb_response_printf( resp, "OK\n" );
    if( !succ )
        nxweb_response_printf( resp, "File not found:%s->%s\n", name_space, fname );

    free(url);
    return NXWEB_OK;
}


nxweb_handler delete_handler={
    .on_request = delete_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};


