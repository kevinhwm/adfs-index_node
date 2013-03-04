
#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

static const char upload_handler_key;
#define UPLOAD_HANDLER_KEY ((nxe_data)&upload_handler_key)

extern KCDB* g_kcdb;
extern int g_kcrecord_header;

#define MAX_COUNT_PER_LIST 10000


typedef struct KC_DATA{
    void * data_ptr;
}KC_DATA;


static void upload_request_data_finalize(
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


static nxweb_result download_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
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
        snprintf( err_msg, sizeof(err_msg)-1, 
                "<html><body>File not found<br/>namespace:%s<br/>filename:%.*s</body></html>", 
                name_space, fname_len, fname );
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
    nxweb_set_request_data(req, UPLOAD_HANDLER_KEY, (nxe_data)(void *)ptmp, upload_request_data_finalize);
    ptmp->data_ptr = pfile_data;

    nxweb_send_data( resp, pfile_data, file_size, "application/octet-stream" );

    free(url);
    return NXWEB_OK;
}


static void fetch_request_data_finalize(
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

static nxweb_result fetch_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("fetch - request\n");
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
    int fname_len = strip_args ? (strip_args-fname) : strlen( fname );
    char fname_with_space[1024]= {0};
    if( strlen( url ) > sizeof( fname_with_space )-1 ){
        char err_msg[1024] = {0};
        snprintf( err_msg, sizeof(err_msg)-1, "File not found<br/>namespace:%s<br/>filename:%.*s" , name_space, fname_len, fname);
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
    void *pfile_data = kcdbseize( g_kcdb, fname_with_space, strlen( fname_with_space), &file_size );
    if( pfile_data == NULL ){
        char err_msg[1024] = {0};
        snprintf( err_msg, sizeof(err_msg)-1, "File not found<br/>namespace:%s<br/>filename:%.*s", name_space, fname_len, fname);
        nxweb_send_http_error( resp, 404, err_msg );
        free(url);
        return NXWEB_MISS;
    }

    KC_DATA *ptmp = nxb_alloc_obj(req->nxb, sizeof(KC_DATA));
    nxweb_set_request_data(req, UPLOAD_HANDLER_KEY, (nxe_data)(void *)ptmp, fetch_request_data_finalize);
    ptmp->data_ptr = pfile_data;

    nxweb_send_data( resp, pfile_data, file_size, "application/octet-stream" );    
    
    free(url);
    return NXWEB_OK;
}


static nxweb_result delete_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("delete - request");

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

static nxweb_result delete_batch_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("delete-batch - request\n");

    nxweb_set_response_content_type(resp, "text/plain");

    // get the namespace. defaults using 
    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    const char *fname_raw = nx_simple_map_get_nocase( req->parameters, "names" );
    if (!fname_raw) {
        nxweb_response_printf(resp, "url error\nexample: http://127.0.0.1:8055/deletefiles?names=f1[:f2][&namespace=np]\n");
        return NXWEB_ERROR;
    }

    KCSTR  kc_fnames[400] = {0};
    char fnames_with_space[400][1024] = {0};
    char *pC = (char *)fname_raw;
    int fname_cnt = 0;
    int fname_len = 0;
    do{
        char *pT = strstr( pC, ":" );
        if( pT ){
            fname_len = pT - pC;
            pT++;
        }
        else{
            fname_len = strlen( pC );
        }
        if( fname_len == 0 ){
            pC = pT;
            continue;
        }
        printf("CurrPC:%s\n", pC );
        if( name_space )
            strcpy( fnames_with_space[fname_cnt], name_space );
        strcat( fnames_with_space[fname_cnt], ":/" );
        strncat( fnames_with_space[fname_cnt], pC, fname_len );
        kc_fnames[fname_cnt].buf = fnames_with_space + fname_cnt;
        kc_fnames[fname_cnt].size = strlen( fnames_with_space[fname_cnt] );
        pC = pT;
        fname_cnt++;
    } while(pC);

    int i = 0;
    for( ; i< fname_cnt; i++ ){
        char pBuf[1024] = {0};
        memcpy( pBuf,  kc_fnames[i].buf, kc_fnames[i].size );
        printf("Found filename:%s from %s\n", pBuf, fnames_with_space[i] );
    }

    int32_t succ = kcdbremovebulk( g_kcdb, kc_fnames, fname_cnt, 0 );
    nxweb_response_printf( resp, "OK\n" );
    if( !succ )
        //nxweb_response_printf( resp, "File not found:%s->%s\n", name_space, fname );
        return NXWEB_OK;
}


/////////////// status /////////////////////
static nxweb_result status_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("status - request");
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


/////////////// list all names /////////////////////
static nxweb_result list_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("list - request\n");

    nxweb_set_response_content_type(resp, "text/plain");
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    const char *prefix = nx_simple_map_get_nocase( req->parameters, "prefix" );
    char kc_prefix[1024] = {0};
    if( name_space ){
        strncpy( kc_prefix, name_space, sizeof( kc_prefix )-3 );
        strcat( kc_prefix, ":/" );
        if( prefix ){
            strncat( kc_prefix, prefix, sizeof(kc_prefix) - strlen( kc_prefix ) - 1 );
        }
    }
    printf("list prefix:%s\n", kc_prefix );
    char *fnames[MAX_COUNT_PER_LIST] = {0};
    int cnt = kcdbmatchprefix (g_kcdb, kc_prefix, fnames, MAX_COUNT_PER_LIST );
    int i = 0;

    while( i< cnt ){
        nxweb_response_printf( resp, "%s\n", fnames[i] );
        kcfree( fnames[i] );
        fnames[i] = NULL;
        i++;
    }
    return NXWEB_OK;
}


nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

nxweb_handler fetch_handler={
    .on_request = fetch_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

nxweb_handler delete_handler={
    .on_request = delete_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

nxweb_handler delete_batch_handler={
    .on_request = delete_batch_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

nxweb_handler status_handler={
    .on_request = status_on_request};

nxweb_handler list_handler={
    .on_request = list_on_request,
    .flags = NXWEB_PARSE_PARAMETERS};

