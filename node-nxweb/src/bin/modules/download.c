#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static const char download_handler_key;
#define DOWNLOAD_HANDLER_KEY ((nxe_data)&download_handler_key)


extern KCDB* g_kcdb;

typedef struct KC_DATA{
    void * data_ptr;
}KC_DATA;


static void download_request_data_finalize(
        nxweb_http_server_connection *conn,
        nxweb_http_request *req,
        nxweb_http_response *resp,
        nxe_data data)
{
    KC_DATA * d = data.ptr;
    if( d->data_ptr ){
        kcfree( d->data_ptr );
        d->data_ptr = NULL;
    }
}


static nxweb_result download_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    printf("download - request\n");
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

    int fname_len = strip_args ? (strip_args - fname) : strlen(fname);
    char fname_with_space[1024] = {0};
    if ( strlen(url) > sizeof(fname_with_space)-1 ) {
        char err_msg[1024] = {0};
        snprintf( err_msg, sizeof(err_msg)-1, "<html><body>File not found<br/>namespace:%s<br/>filename:%.*s</body></html>", name_space, fname_len, fname );
        nxweb_send_http_error( resp, 404, err_msg );
        free(url);
        return NXWEB_MISS;
    }

    if( name_space ) 
        sprintf( fname_with_space, "%s:/", name_space);
    else
        strcpy( fname_with_space, ":/" );

    strncat( fname_with_space, fname, fname_len );

    size_t file_size = 0;
    void *pfile_data = kcdbget( g_kcdb, fname_with_space, strlen(fname_with_space), &file_size );
    if( pfile_data == NULL ){
        char err_msg[1024] = {0};
        snprintf( err_msg, sizeof(err_msg)-1, 
                "File not found<br/>namespace:%s<br/>filename:%.*s", 
                name_space, fname_len, fname );
        nxweb_send_http_error( resp, 404, err_msg );
        free(url);
        return NXWEB_MISS;
    }

    KC_DATA *ptmp = nxb_alloc_obj(req->nxb, sizeof(KC_DATA));
    nxweb_set_request_data(req, DOWNLOAD_HANDLER_KEY, (nxe_data)(void *)ptmp, download_request_data_finalize);
    ptmp->data_ptr = pfile_data;

    nxweb_send_data( resp, pfile_data, file_size, "application/octet-stream" );

    free(url);
    return NXWEB_OK;
}


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


static nxweb_result status_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("status - request\n");
    nxweb_set_response_content_type(resp, "text/plain");
    char *status = kcdbstatus( g_kcdb );

    size_t len = 2*strlen(status);
    char *buf = malloc(len);
    memset(buf, 0, len);

    int i=0, j=0;
    for (i=0; i<strlen(status); i++) {
        if (status[i] == '\t') {
            strcat(buf, ": ");
            j += 2;
        }
        else {
            buf[j] = status[i];
            j++;
        }
    }

    nxweb_response_printf( resp, "%s\n", buf );
    kcfree( status );
    status = NULL;
    free(buf);
    return NXWEB_OK;
}


nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

nxweb_handler delete_handler={
    .on_request = delete_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

nxweb_handler status_handler={
    .on_request = status_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

