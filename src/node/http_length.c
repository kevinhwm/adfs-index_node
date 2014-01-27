/* http_length.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
#include "kclangc.h"
#include "manager.h"


static nxweb_result length_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    if (strlen(req->path_info) >= _DFS_MAX_LEN) {
	nxweb_send_http_error(resp, 400, "Failed. File name is too long.");
	return NXWEB_ERROR;
    }
    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    char fname[_DFS_MAX_LEN] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, NULL);
    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
    if (get_filename_from_url(fname, pattern) < 0) {
	nxweb_send_http_error(resp, 400, "Failed. Check file name.");
	return NXWEB_ERROR;
    }

    char msg[1024] = {0};
    unsigned long file_len = 0;
    if (GNm_length(name_space, fname, &file_len) == -1) {
	nxweb_send_http_error(resp, 404, "Failed. No file.");
	snprintf(msg, sizeof(msg), "[%s:%s]->error.[%s]", name_space, fname, conn->remote_addr);
	log_out("length", msg, LOG_LEVEL_INFO);
	return NXWEB_ERROR;
    }
    else {
	char buf[128] = {0};
	sprintf(buf, "%lu", file_len);
	nxweb_response_printf(resp, buf);
	snprintf(msg, sizeof(msg), "[%s:%s]->ok.[%s]", name_space, fname, conn->remote_addr);
	log_out("length", msg, LOG_LEVEL_INFO);
	return NXWEB_OK;
    }
}

nxweb_handler length_handler={
    .on_request = length_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

