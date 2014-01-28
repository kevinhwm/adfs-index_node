/* http_exist.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include <string.h>
#include "manager.h"

static nxweb_result exist_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    if (strlen(req->uri) >= _DFS_MAX_LEN) {
	nxweb_send_http_error(resp, 414, "Request-URI Too Long");
	resp->keep_alive=0;
	return NXWEB_ERROR;
    }
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_parse_request_parameters(req, 0);
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );

    char fname[_DFS_MAX_LEN] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, NULL);
    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
    if (get_filename_from_url(fname, pattern) < 0) {
	nxweb_send_http_error(resp, 403, "Forbidden\nCheck file name");
	resp->keep_alive = 0;
	return -1;
    }

    if (strlen(fname) >= _DFS_FILENAME_LEN) {
	nxweb_send_http_error(resp, 403, "Forbidden\nFile name is too long. It must be less than 250");
	resp->keep_alive = 0;
	return -1;
    }

    char msg[1024] = {0};
    
    if (GIm_exist(name_space, fname)) {
	snprintf(msg, sizeof(msg), "[%s:%s]->ok.[%s]", name_space, fname, conn->remote_addr);
	log_out("exist", msg, LOG_LEVEL_INFO);
	nxweb_response_printf(resp, "OK" );
	return NXWEB_OK;
    }
    else {
	nxweb_send_http_error(resp, 404, "Not Found");
	resp->keep_alive = 0;
	snprintf(msg, sizeof(msg), "[%s:%s]->no file.[%s]", name_space, fname, conn->remote_addr);
	log_out("exist", msg, LOG_LEVEL_INFO);
	return -1;
    }
}

nxweb_handler exist_handler={
    .on_request = exist_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

