/* if_status.c
 *
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include "nxweb/nxweb.h"
//#include <kclangc.h>
//#include <unistd.h>
#include <string.h>
#include "ai.h"


static nxweb_result status_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_response_append_str(resp, "<html><head><title>ADFS - status</title></head><body>");
    nxweb_response_append_str(resp, "<h3>ADFS status table</h3>");
    nxweb_response_append_str(resp, "<li>zone1</li>");
    nxweb_response_append_str(resp, "<table border>");
    nxweb_response_append_str(resp, "<tr><th>node</th><th>status</th><th>file number</th><th>kch number</th></tr>");
    nxweb_response_append_str(resp, "<tr><td>10.0.11.222:9527</td><td bgcolor=\"red\">ok</td><td>30341</td><td>4</td></tr>");
    nxweb_response_append_str(resp, "<tr><td>10.0.11.223:9527</td><td bgcolor=\"green\">ok</td><td>30341</td><td>4</td></tr>");
    nxweb_response_append_str(resp, "</table></body></html>");
    nxweb_response_append_str(resp, "</body></html>");
    return NXWEB_OK;
}

nxweb_handler status_handler={
    .on_request = status_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

