/* Antiy Labs. Basic Platform R & D Center
 * if_delete.c
 *
 * huangtao@antiy.com
 */

#include "nxweb/nxweb.h"
#include <kclangc.h>
#include "ai.h"


static nxweb_result delete_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    char fname[ADFS_URL_PATH] = {0};
    const char *name_space = NULL;

    DBG_PRINTS("delete - request\n");
    if (strlen(req->path_info) >= ADFS_URL_PATH)
    {
        nxweb_send_http_error(resp, 400, MSG_ERR_LONG_URL);
        return NXWEB_ERROR;
    }
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_parse_request_parameters(req, 0);
    name_space = nx_simple_map_get_nocase(req->parameters, "namespace");
    strncpy(fname, req->path_info, sizeof(fname));
    if (parse_filename(fname) == ADFS_ERROR)
    {
        nxweb_send_http_error(resp, 400, MSG_ERR_ILLEGAL_NAME);
        return NXWEB_ERROR;
    }

    if (mgr_delete(name_space, fname) == ADFS_ERROR)
    {
        nxweb_send_http_error(resp, 404, "Failed. No file");
        return NXWEB_ERROR;
    }
    else
    {
        nxweb_response_printf(resp, "OK.\n");
        return NXWEB_OK;
    }
}

nxweb_handler delete_handler={
    .on_request = delete_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

