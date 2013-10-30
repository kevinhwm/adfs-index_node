/* f_download.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include <string.h>
#include "manager.h"

static nxweb_result download_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    if (strlen(req->uri) >= ADFS_MAX_LEN) {
	nxweb_send_http_error(resp, 414, "Request-URI Too Long");
	resp->keep_alive=0;
	return NXWEB_ERROR;
    }
    nxweb_parse_request_parameters(req, 0);
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    const char *history = nx_simple_map_get_nocase( req->parameters, "history" );

    char fname[ADFS_MAX_LEN] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, NULL);
    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
    if (get_filename_from_url(fname, pattern) < 0) {
	nxweb_send_http_error(resp, 403, "Forbidden\nCheck file name");
	resp->keep_alive = 0;
	return ADFS_ERROR;
    }

    if (strlen(fname) >= ADFS_FILENAME_LEN) {
	nxweb_send_http_error(resp, 403, "Forbidden\nFile name is too long. It must be less than 250");
	resp->keep_alive = 0;
	return ADFS_ERROR;
    }

    char msg[1024] = {0};
    char *redirect_url = aim_download(name_space, fname, history);
    if (redirect_url == NULL) {
	nxweb_send_http_error(resp, 404, "Not Found");
	resp->keep_alive = 0;
	snprintf(msg, sizeof(msg), "[%s:%s:%s]->no file.[%s]", name_space, fname, history, conn->remote_addr);
	log_out("download", msg, LOG_LEVEL_INFO);
	return ADFS_ERROR;
    }
    else {
	nxweb_send_redirect(resp, 302, redirect_url, conn->secure);
	snprintf(msg, sizeof(msg), "[%s:%s:%s-%s]->ok.[%s]", name_space, fname, history, redirect_url, conn->remote_addr);
	log_out("download", msg, LOG_LEVEL_INFO);
	free(redirect_url);
	return NXWEB_OK;
    }
}

nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

