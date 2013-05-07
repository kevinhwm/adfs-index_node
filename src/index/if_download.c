/* if_download.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
#include <string.h>
#include "manager.h"


static const char download_handler_key;
#define DOWNLOAD_HANDLER_KEY ((nxe_data)&download_handler_key)


static nxweb_result download_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    DBG_PRINTS("download - request\n");
    if (strlen(req->path_info) >= ADFS_MAX_PATH)
    {
        nxweb_send_http_error(resp, 400, "Failed. File name is too long. must less than 1024.");
        return NXWEB_ERROR;
    }

    nxweb_parse_request_parameters( req, 0 );
    const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
    const char *history = nx_simple_map_get_nocase( req->parameters, "history" );

    char fname[ADFS_MAX_PATH] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    if (get_filename_from_url(fname) != 0) {
        nxweb_send_http_error(resp, 400, "Failed. Check file name");
        return NXWEB_OK;
    }

    if (strlen(fname) >= ADFS_FILENAME_LEN) {
	nxweb_send_http_error(resp, 400, "Failed. File name is too long. It must be less than 250\n");
	return NXWEB_OK;
    }

    char *redirect_url = mgr_download(name_space, fname, history);
    if (redirect_url == NULL) {
        nxweb_send_http_error(resp, 404, "Failed. No file");
    }
    else {
        nxweb_send_redirect(resp, 303, redirect_url, conn->secure);
        free(redirect_url);
    }
    return NXWEB_OK;
}

nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

