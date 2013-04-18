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
    DBG_PRINTS("download - request\n");
    if (strlen(req->path_info) >= PATH_MAX)
    {
        nxweb_send_http_error(resp, 400, "Failed. File name is too long");
        return NXWEB_ERROR;
    }

    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );

    char fname[PATH_MAX] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    if (parse_filename(fname) == ADFS_ERROR)
    {
        nxweb_send_http_error(resp, 400, "Failed. Check file name");
        return NXWEB_ERROR;
    }

    DBG_PRINTS("path_info: ");
    DBG_PRINTSN(req->path_info);
    DBG_PRINTS("fname: ");
    DBG_PRINTSN(fname);
    DBG_PRINTS("namespace: ");
    DBG_PRINTSN(name_space);

    void *pfile_data = NULL;
    size_t file_size = 0;

    mgr_get(fname, name_space, &pfile_data, &file_size);    // query db
    if (pfile_data == NULL || file_size == 0)
    {
        nxweb_send_http_error(resp, 404, "Failed. No file");
        return NXWEB_ERROR;
    }
    else
    {
        DBG_PRINTS("download -10\n");
        SHARE_DATA *ptmp = nxb_alloc_obj(req->nxb, sizeof(SHARE_DATA));
        nxweb_set_request_data(req, DOWNLOAD_HANDLER_KEY, (nxe_data)(void *)ptmp, download_request_data_finalize);
        ptmp->data_ptr = pfile_data;

        char *file_name = fname, *tmp = NULL;
        while ((tmp = strstr(file_name, "/")))
            file_name = tmp + 1;

        char resp_name[NAME_MAX] = {0};
        snprintf( resp_name, sizeof(resp_name), "attachment; filename=%s", file_name );
        nxweb_add_response_header(resp, "Content-disposition", resp_name );

        DBG_PRINTS("download -20\n");
        nxweb_send_data( resp, pfile_data, file_size, "application/octet-stream" );
        DBG_PRINTS("download -30\n");
    }

    return NXWEB_OK;
}


nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

