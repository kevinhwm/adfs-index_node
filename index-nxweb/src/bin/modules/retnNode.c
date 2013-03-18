#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>


extern KCDB* g_kcdb;

static nxweb_result retnNode_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    return NXWEB_OK;
}

nxweb_handler retnNode_handler={
    .on_request = retnNode_on_request};
