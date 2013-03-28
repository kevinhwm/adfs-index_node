/*
 *
 */


#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <string.h>


extern KCDB* g_kcdb;

static nxweb_result status_on_request(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) 
{
    printf("status - request\n");
    nxweb_set_response_content_type(resp, "text/plain");
    char *status = kcdbstatus( g_kcdb );

    size_t len = 2*strlen(status);
    char *buf = malloc(len);
    memset(buf, 0, len);

    int i=0, j=0;
    for (i=0; i<strlen(status); i++) {
        if (status[i] == '\t') {
            strcat(buf, ": ");
            j += 2;
        }
        else {
            buf[j] = status[i];
            j++;
        }
    }

    nxweb_response_printf( resp, "%s\n", buf );
    kcfree( status );
    status = NULL;
    free(buf);
    return NXWEB_OK;
}


nxweb_handler status_handler={
    .on_request = status_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

