#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>


extern KCDB* g_kcdb;

static nxweb_result gethistory_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    return NXWEB_OK;
}

nxweb_handler gethistory_handler={
    .on_request = gethistory_on_request};
