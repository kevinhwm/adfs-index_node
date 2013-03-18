#include "nxweb/nxweb.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>


extern KCDB* g_kcdb;


static nxweb_result status_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp) 
{
    printf("status - request");
    nxweb_set_response_content_type(resp, "text/plain");
    char *status = kcdbstatus( g_kcdb );

    size_t len = 2*strlen(status);
    char *buf = malloc(len);
    memset(buf, 0, len);

    int i=0, j=0;
    for (i=0; i<strlen(status); i++) 
    {
        if (status[i] == '\t') 
        {
            strcat(buf, ": ");
            j += 2;
        }
        else 
        {
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
    .on_request = status_on_request};

/*
  NXWEB_INPROCESS=0, // execute handler in network thread (must be fast and non-blocking!)
  NXWEB_INWORKER=1, // execute handler in worker thread (for lengthy or blocking operations)
  NXWEB_PARSE_PARAMETERS=2, // parse query string and (url-encoded) post data before calling this handler
  NXWEB_PRESERVE_URI=4, // modifier for NXWEB_PARSE_PARAMETERS; preserver conn->uri string while parsing (allocate copy)
  NXWEB_PARSE_COOKIES=8, // parse cookie header before calling this handler
  NXWEB_HANDLE_GET=0x10,
  NXWEB_HANDLE_POST=0x20, // implies NXWEB_ACCEPT_CONTENT
  NXWEB_HANDLE_OTHER=0x40,
  NXWEB_HANDLE_ANY=0x70,
  NXWEB_ACCEPT_CONTENT=0x80, // handler accepts request body
  _NXWEB_HANDLE_MASK=0x70
*/
