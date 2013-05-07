/*
 * huangtao@antiy.com
 */

#include "nxweb/nxweb.h"

static nxweb_result status_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    nxweb_response_printf(resp, "OK. Alive");
    return NXWEB_OK;
}

nxweb_handler status_handler={
    .on_request = status_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

