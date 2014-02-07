/* http_status.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include <string.h>
#include "manager.h"


static nxweb_result status_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_response_append_str(resp, "<html><head><title>status</title></head><body>");

    char *pinfo = GIm_status();
    nxweb_response_append_str(resp, pinfo);
    nxweb_response_append_str(resp, "</body></html>");
    free(pinfo);
    return NXWEB_OK;
}

nxweb_handler status_handler={
    .on_request = status_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

