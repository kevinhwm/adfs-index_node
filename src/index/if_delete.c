/* if_delete.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
#include "ai_manager.h"


static nxweb_result delete_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    char fname[ADFS_MAX_PATH] = {0};
    const char *name_space = NULL;

    DBG_PRINTS("delete - request\n");
    if (strlen(req->path_info) >= ADFS_MAX_PATH) {
        nxweb_send_http_error(resp, 400, "Failed. Url is too long.");
	resp->keep_alive = 0;
        return NXWEB_ERROR;
    }
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_parse_request_parameters(req, 0);
    name_space = nx_simple_map_get_nocase(req->parameters, "namespace");
    strncpy(fname, req->path_info, sizeof(fname));
    if (get_filename_from_url(fname) != 0) {
        nxweb_send_http_error(resp, 400, "Failed. File name is illegal.");
	resp->keep_alive = 0;
        return NXWEB_ERROR;
    }

    if (aim_delete(name_space, fname) == ADFS_ERROR) {
        nxweb_send_http_error(resp, 404, "Failed. No file");
	resp->keep_alive = 0;
        return NXWEB_ERROR;
    }
    else {
        nxweb_response_printf(resp, "OK.\n");
        return NXWEB_OK;
    }
}

nxweb_handler delete_handler={
    .on_request = delete_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

