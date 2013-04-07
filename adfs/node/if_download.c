/*
 * huangtao@antiy.com
 */

#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include "an.h"

static const char download_handler_key;
#define DOWNLOAD_HANDLER_KEY ((nxe_data)&download_handler_key)


typedef struct SHARE_DATA{
    void * data_ptr;
}SHARE_DATA;


static void download_request_data_finalize(
        nxweb_http_server_connection *conn,
        nxweb_http_request *req,
        nxweb_http_response *resp,
        nxe_data data)
{
    SHARE_DATA * d = data.ptr;
    if( d->data_ptr )
    {
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

    char fname[MAX_URL_LENGTH] = {0};

    printf("download - 1\n");
    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    printf("download - 2\n");

    /////
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, 0);

    if (strlen(fname) <2 || fname[0] == '?')
    {
        nxweb_send_http_error(resp, 404, "Node: File Not Found");
        return NXWEB_OK;
    }
    char *p = strstr(fname, "?");
    if (p != NULL)
    {
        p[0] = '\0';
        if (fname[p-fname-1] == '/')
            fname[p-fname-1] = '\0';
    }
    /////

    printf("path_info: %s\n", req->path_info);
    printf("fname: %s\n", fname);
    printf("namespace: %s\n", name_space);

    void *pfile_data = NULL;
    size_t file_size = 0;

    /*
    char fname[2048] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    if (parse_filename(fname) == ADFS_ERROR)
        nxweb_response_printf( resp, "Failed\n" );
    */

    mgr_download(fname, name_space, &pfile_data, &file_size);    // query db
    if (pfile_data == NULL)
    {
        nxweb_send_http_error(resp, 404, "Node: Failed");
        return NXWEB_ERROR;
    }
    else
    {
        SHARE_DATA *ptmp = nxb_alloc_obj(req->nxb, sizeof(SHARE_DATA));
        nxweb_set_request_data(req, DOWNLOAD_HANDLER_KEY, (nxe_data)(void *)ptmp, download_request_data_finalize);
        ptmp->data_ptr = pfile_data;

        nxweb_send_data( resp, pfile_data, file_size, "application/octet-stream" );
    }

    return NXWEB_OK;
}


nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

