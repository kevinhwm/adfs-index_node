/* if_syn.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
#include "an_manager.h"

// just delete records in index.kch
// monitor thread delete records which are not in index.kch 
static nxweb_result syn_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    char msg[1024] = {0};

    if (anm_syn() == ADFS_ERROR) {
	nxweb_send_http_error(resp, 400, "Failed");
	resp->keep_alive = 0;
	snprintf(msg, sizeof(msg), "syn error[%s]", conn->remote_addr);
	log_out("syn", msg, LOG_LEVEL_INFO);
	return NXWEB_ERROR;
    }
    else {
	nxweb_response_printf(resp, "OK.");
	snprintf(msg, sizeof(msg), "syn ok[%s]", conn->remote_addr);
	log_out("syn", msg, LOG_LEVEL_INFO);
	return NXWEB_OK;
    }
}

nxweb_handler syn_handler={
    .on_request = syn_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

