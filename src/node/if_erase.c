/* if_erase.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include "an_manager.h"

// just delete records in index.kch
// monitor thread delete records which are not in index.kch 
static nxweb_result erase_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    if (strlen(req->path_info) >= ADFS_MAX_PATH) {
	nxweb_send_http_error(resp, 400, "Failed. File name is too long.");
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }
    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    char fname[ADFS_MAX_PATH] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, NULL);
    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
    if (get_filename_from_url(fname, pattern) < 0) {
	nxweb_send_http_error(resp, 400, "Failed. Check file name.");
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }

    char msg[1024] = {0};
    if (anm_erase(name_space, fname) == ADFS_ERROR) {
	nxweb_send_http_error(resp, 400, "Failed.");
	resp->keep_alive = 0;
	snprintf(msg, sizeof(msg), "[%s:%s]->no file.[%s]", name_space, fname, conn->remote_addr);
	log_out("erase", msg, LOG_LEVEL_INFO);
	return NXWEB_ERROR;
    }
    else {
	nxweb_response_printf(resp, "OK.");
	snprintf(msg, sizeof(msg), "[%s:%s]->ok.[%s]", name_space, fname, conn->remote_addr);
	log_out("erase", msg, LOG_LEVEL_INFO);
	return NXWEB_OK;
    }
}

nxweb_handler erase_handler={
    .on_request = erase_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

