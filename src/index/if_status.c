/* if_status.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
#include <string.h>
#include "ai_manager.h"

static const char status_handler_key; 
#define STATUS_HANDLER_KEY ((nxe_data)&status_handler_key)

struct SHARE_DATA {
    char *p;
};

static void status_request_data_finalize(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp, 
	nxe_data data) 
{
    struct SHARE_DATA * tmp = data.ptr;
    if (tmp && tmp->p) {
	free(tmp->p);
	tmp->p = NULL;
    }
}

static nxweb_result status_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_response_append_str(resp, "<html><head><title>ADFS - status</title></head><body>");
    nxweb_response_append_str(resp, "<h3>ADFS status table</h3>");

    struct SHARE_DATA *tmp = nxb_alloc_obj(req->nxb, sizeof(struct SHARE_DATA));
    nxweb_set_request_data(req, STATUS_HANDLER_KEY, (nxe_data)(void*)tmp, status_request_data_finalize);
    char *p = aim_status();
    tmp->p = p;
    nxweb_response_append_str(resp, p);
    nxweb_response_append_str(resp, "</body></html>");
    return NXWEB_OK;
}

nxweb_handler status_handler={
    .on_request = status_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

