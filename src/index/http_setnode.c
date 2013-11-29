/* http_setnode.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include <string.h>
#include "manager.h"

extern CIManager g_manager;

static nxweb_result setnode_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    if ( !g_manager.primary ) {
        nxweb_send_http_error(resp, 400, "Bad Request");
	resp->keep_alive = 0;
        return NXWEB_ERROR;
    }

    if (strlen(req->uri) >= _DFS_MAX_LEN) {
	nxweb_send_http_error(resp, 414, "Request-URI Too Long");
	resp->keep_alive=0;
	return NXWEB_ERROR;
    }

    nxweb_parse_request_parameters(req, 0);
    const char *node_name = nx_simple_map_get_nocase(req->parameters, "node");
    const char *attr_rw = nx_simple_map_get_nocase(req->parameters, "attr");
    if (node_name == NULL || attr_rw == NULL) {goto err1;}
    if (GIm_setnode(node_name, attr_rw) < 0) {goto err1;}

    char msg[1024] = {0};
    snprintf(msg, sizeof(msg), "[%s:%s]->setnode ok.[%s]", node_name, attr_rw, conn->remote_addr);
    log_out("setnode", msg, LOG_LEVEL_INFO);
    nxweb_response_append_str(resp, "OK");
    return NXWEB_OK;

err1:
    snprintf(msg, sizeof(msg), "[%s:%s]->setnode error.[%s]", node_name, attr_rw, conn->remote_addr);
    log_out("setnode", msg, LOG_LEVEL_INFO);
    nxweb_send_http_error(resp, 403, "Forbidden");
    resp->keep_alive = 0;
    return NXWEB_ERROR;
}

nxweb_handler setnode_handler={
    .on_request = setnode_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

