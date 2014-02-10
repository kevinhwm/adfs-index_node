/* http_delete.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include "manager.h"


static nxweb_result delete_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    if (strlen(req->path_info) >= _DFS_MAX_LEN) {
        nxweb_send_http_error(resp, 414, "Request-URI Too Long");
	resp->keep_alive = 0;
        return NXWEB_ERROR;
    }
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_parse_request_parameters(req, 0);
    const char *name_space = nx_simple_map_get_nocase(req->parameters, "namespace");

    char fname[_DFS_MAX_LEN] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, NULL);
    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
    if (get_filename_from_url(fname, pattern) < 0) {
        nxweb_send_http_error(resp, 403, "Forbidden");
	resp->keep_alive = 0;
        return NXWEB_ERROR;
    }

    char msg[1024] = {0};
    if (GIm_delete(name_space, fname) < 0) {
        nxweb_send_http_error(resp, 500, "Internal Server Error");
	resp->keep_alive = 0;
	snprintf(msg, sizeof(msg), "[%s:%s]->no file.[%s]", name_space, fname, conn->remote_addr);
	log_out("delete", msg, LOG_LEVEL_INFO);
        return NXWEB_ERROR;
    }
    else {
        nxweb_response_printf(resp, "OK.\n");
	snprintf(msg, sizeof(msg), "[%s:%s]->ok.[%s]", name_space, fname, conn->remote_addr);
	log_out("delete", msg, LOG_LEVEL_INFO);
        return NXWEB_OK;
    }
}

nxweb_handler delete_handler={
    .on_request = delete_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

