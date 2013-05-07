/* if_remove.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
#include "an_manager.h"


static nxweb_result remove_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    DBG_PRINTS("remove - request\n");
    if (strlen(req->path_info) >= ADFS_MAX_PATH) {
	nxweb_send_http_error(resp, 400, "Failed. File name is too long.");
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }
    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    char fname[ADFS_MAX_PATH] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    if (get_filename_from_url(fname) == ADFS_ERROR) {
	nxweb_send_http_error(resp, 400, "Failed. Check file name.");
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }

    if (anm_remove(name_space, fname) == ADFS_ERROR) {
	nxweb_send_http_error(resp, 404, "Failed. Not found");
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }
    else {
	nxweb_response_printf(resp, "OK.");
	return NXWEB_OK;
    }
}

nxweb_handler remove_handler={
    .on_request = remove_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

