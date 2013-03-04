/*
 * Copyright (c) 2011-2012 Yaroslav Stavnichiy <yarosla@gmail.com>
 *
 * This file is part of NXWEB.
 *
 * NXWEB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * NXWEB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with NXWEB. If not, see <http://www.gnu.org/licenses/>.
 */

#include "nxweb/nxweb.h"
#include "../post_parser/multipart_parser.h"
#include <kclangc.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#define MAX_UPLOAD_SIZE (1048576*200)
extern KCDB* g_kcdb;
extern int g_kcrecord_header;

static const char upload_handler_key; // variable's address only matters

typedef struct _upload_file_object{
    nxd_fwbuffer              fwbuffer;
    void *                    postdata_ptr;
    size_t  	              postdata_len;
    char                      post_boundary[1024];
    FILE *                    fpostmem;
    int                       filename_ready_to_receive;
    char                      filename[512];
    int                       file_ready_to_receive;
    int                       file_complete;
    void *                    file_ptr;
    size_t                    file_len;
    FILE *					  ffilemem;
    multipart_parser_settings parser_settings;
    multipart_parser *        parser;
    char                      key[1024];
    char                      value[1024];
    KCDB *                    kc_db;
}upload_file_object;

#define UPLOAD_HANDLER_KEY ((nxe_data)&upload_handler_key)


static int on_post_header_field( multipart_parser *mp_obj, const char *at, size_t length )
{
    char buf[1024] = {0};
    strncpy( buf, at, length );
    //printf("recv header_field: length:%lu \ndata is:%s\n-----end------\n\n", length, buf );

    return 0;
}

static int on_post_header_value( multipart_parser *mp_obj, const char *at, size_t length )
{
    char buff[1024] = {0};
    strncpy( buff, at, length );
    //printf("recv header_value: length:%lu\ndata is:%s\n------end------\n\n", length, buff );
    upload_file_object *pufo = multipart_parser_get_data( mp_obj );
    char *pfname = strstr( buff, "filename=\"" );
    if( pfname == NULL )
    {
        if( strstr( buff, "name=\"upname\"" ) ){
            pufo->filename_ready_to_receive = 1;
            pufo->file_ready_to_receive = 0;
            return 0;
        }
        else{
            //printf("Not found filename.\n");
            return 0;
        }
    }

    pfname += 10;
    char *pfname_end = strstr( pfname+1, "\"" );
    strncpy( pufo->filename, pfname, pfname_end - pfname );
    //printf( "Found filename:\"%s\"\n", pufo->filename );
    pufo->file_ready_to_receive = 1;
    return 0;
}

static int on_post_body( multipart_parser *mp_obj, const char *at, size_t len )
{
    int length = len;
    int i;
    //printf("recv post body:length: %d\n", length );

    /*
    if( length < 100 )
    {
        char buff[1024] = {0};
        strncpy( buff, at, length+4 );
        printf("recv post body:%s\n-------------------end-----------------\n\n", buff );
    }
    */

    upload_file_object *pufo = multipart_parser_get_data( mp_obj );
    if( pufo->filename_ready_to_receive == 1 ){
        strncpy( pufo->filename, at, length );
        pufo->filename_ready_to_receive = 0;
        //printf("Found filename:%s\n", pufo->filename );
        pufo->file_ready_to_receive = 1;
        return 0;
    }
    if( pufo->file_ready_to_receive ){
        if( pufo->ffilemem == NULL ) {
            return 0;
        }
        if( pufo->ffilemem )
            fwrite( at, 1, length, pufo->ffilemem);
        //printf("Write to file body %d.", length );
    }
    return 0;
}


int on_post_finished (multipart_parser * mp_obj)
{
    upload_file_object *pufo = multipart_parser_get_data( mp_obj );
    if( pufo->ffilemem )
    {
        fclose( pufo->ffilemem );
        pufo->file_complete = 1;
        pufo->ffilemem=NULL;
    }
    //printf("Post finished.\n" );
    return 0;
}



static nxweb_result upload_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp)
{ 
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_set_response_charset(resp, "utf-8" );

    nxweb_response_append_str(resp, "<html><head><title>Upload Module</title></head><body>\n");

    upload_file_object *ufo = nxweb_get_request_data(req, UPLOAD_HANDLER_KEY).ptr;
    nxd_fwbuffer* fwb= &ufo->fwbuffer;

    if (fwb) {
        ufo->parser_settings.on_header_field = on_post_header_field;
        ufo->parser_settings.on_header_value = on_post_header_value;
        ufo->parser_settings.on_part_data = on_post_body;
        ufo->parser_settings.on_body_end = on_post_finished;
        ufo->parser = multipart_parser_init( ufo->post_boundary, &ufo->parser_settings );

        // get the namespace. defaults using ""
        nxweb_parse_request_parameters( req, 0 );
        const char *name_space = nx_simple_map_get_nocase( req->parameters, "namespace" );
        printf("ns:%s\n", name_space);

        multipart_parser_set_data( ufo->parser, ufo );
        ufo->ffilemem = open_memstream((char**) &ufo->file_ptr, &ufo->file_len );
        //int time_stamp = (int)time(NULL);
        //fwrite( &time_stamp, 1, g_kcrecord_header, ufo->ffilemem );

        multipart_parser_execute( ufo->parser, ufo->postdata_ptr, ufo->postdata_len );
        multipart_parser_free( ufo->parser );
        if( strlen( ufo->filename ) > 0 && ufo->file_complete ){
            char name_with_space[1024]= {0};
            if( name_space )
                sprintf( name_with_space, "%s:/%s", name_space, ufo->filename );
            else
                sprintf( name_with_space, ":/%s", ufo->filename );

            printf("[%8lu][%8lu]\t[%s]\n", 
                    strlen(name_with_space), ufo->file_len, name_with_space );
            if( !kcdbset( g_kcdb, name_with_space, strlen( name_with_space ), ufo->file_ptr, ufo->file_len ) )
                nxweb_response_printf( resp, "store backend db failed.\n" );
            else
            {
                nxweb_response_printf( resp, "OK\n");
                nxweb_response_printf( resp, "Filename:%s\n", ufo->filename );
                nxweb_response_printf( resp, "FileLength:%d\n", ufo->file_len );
            }
        }
        else
            printf("File not received:filename(%s)\n", ufo->filename );

        if( ufo->file_ptr ){
            free( ufo->file_ptr );
            ufo->file_ptr = NULL;
        }
    }

    nxweb_response_printf(resp, "<form method='post' enctype='multipart/form-data'>File(s) to upload: <input type='file' multiple name='uploadedfile' /> </br>  <input type='submit' value='UploadFile!' /></form>\n");
    nxweb_response_printf(resp, ""
            "<form method=\"POST\" enctype=\"multipart/form-data\" action=\"\">"
            "upname:<br />"
            "<input type=\"text\" name=\"upname\" />"
            "</br>"
            "data:<br/>"
            "<textarea name=\"data\" cols=100 rows=20 >"
            "input your data here..."
            "</textarea>"
            "<br />"
            "<input type=\"submit\" value=\'UploadForm\' />"
            "</form>");

    /// Special %-printf-conversions implemeted by nxweb:
    //    %H - html-escape string
    //    %U - url-encode string
    nxweb_response_printf(resp, "<p>Received request:</p>\n<blockquote>"
            "remote_addr=%H<br/>\n"
            "method=%H<br/>\n"
            "uri=%H<br/>\n"
            "path_info=%H<br/>\n"
            "http_version=%H<br/>\n"
            "http11=%d<br/>\n"
            "ssl=%d<br/>\n"
            "keep_alive=%d<br/>\n"
            "host=%H<br/>\n"
            "cookie=%H<br/>\n"
            "user_agent=%H<br/>\n"
            "content_type=%H<br/>\n"
            "content_length=%ld<br/>\n"
            "content_received=%ld<br/>\n"
            "transfer_encoding=%H<br/>\n"
            "accept_encoding=%H<br/>\n"
            "request_body=%H</blockquote>\n",
            conn->remote_addr,
            req->method,
            req->uri,
            req->path_info,
            req->http_version,
            req->http11,
            conn->secure,
            req->keep_alive,
            req->host,
            req->cookie,
            req->user_agent,
            req->content_type,
            req->content_length,
            req->content_received,
            req->transfer_encoding,
            req->accept_encoding,
            req->content
                );

    if (req->headers) {
        nxweb_response_append_str(resp, "<h3>Headers:</h3>\n<ul>\n");
        nx_simple_map_entry* itr;
        for (itr=nx_simple_map_itr_begin(req->headers); itr; itr=nx_simple_map_itr_next(itr)) {
            nxweb_response_printf(resp, "<li>%H=%H</li>\n", itr->name, itr->value);
        }
        nxweb_response_append_str(resp, "</ul>\n");
    }

    if (req->parameters) {
        nxweb_response_append_str(resp, "<h3>Get parameters:</h3>\n<ul>\n");
        nx_simple_map_entry* itr;
        for (itr=nx_simple_map_itr_begin(req->parameters); itr; itr=nx_simple_map_itr_next(itr)) {
            nxweb_response_printf(resp, "<li>%H=%H</li>\n", itr->name, itr->value);
        }
        nxweb_response_append_str(resp, "</ul>\n");
    }

    nxweb_response_append_str(resp, "</body></html>");

    return NXWEB_OK;
}


static void upload_request_data_finalize(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp, nxe_data data) {
    upload_file_object *ufo = data.ptr;
    nxd_fwbuffer* fwb= &ufo->fwbuffer;
    if (fwb && fwb->fd) {
        fclose(fwb->fd);
        fwb->fd=0;
    }

    if( ufo->postdata_ptr )
    {
        free( ufo->postdata_ptr );
        ufo->postdata_ptr = NULL;
    }
}

static nxweb_result upload_on_post_data(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) {

    if (req->content_length>MAX_UPLOAD_SIZE) {
        nxweb_send_http_error(resp, 413, "Request Entity Too Large");
        resp->keep_alive=0; // close connection
        nxweb_start_sending_response(conn, resp);
        return NXWEB_OK;
    }
    upload_file_object* ufo=nxb_alloc_obj(req->nxb, sizeof(upload_file_object));
    memset( ufo, 0, sizeof( upload_file_object ) );

    nxd_fwbuffer* fwb = &ufo->fwbuffer;
    nxweb_set_request_data(req, UPLOAD_HANDLER_KEY, (nxe_data)(void*)ufo, upload_request_data_finalize);

    sscanf(req->content_type, "%*[^=]%*1s%s", ufo->post_boundary+2);
    ufo->post_boundary[0]='-';
    ufo->post_boundary[1]='-';
    //printf("boundary=%s\n\n", ufo->post_boundary );

    ufo->fpostmem = open_memstream( (char **)&ufo->postdata_ptr, &ufo->postdata_len );
    nxd_fwbuffer_init(fwb, ufo->fpostmem, MAX_UPLOAD_SIZE);
    conn->hsp.cls->connect_request_body_out(&conn->hsp, &fwb->data_in);
    conn->hsp.cls->start_receiving_request_body(&conn->hsp);
    return NXWEB_OK;
}

static nxweb_result upload_on_post_data_complete(nxweb_http_server_connection* conn, nxweb_http_request* req, nxweb_http_response* resp) {
    // It is not strictly necessary to close the file here
    // as we are closing it anyway in request data finalizer.
    // Releasing resources in finalizer is the proper way of doing this
    // as any other callbacks might not be invoked under error conditions.
    upload_file_object* ufo = nxweb_get_request_data(req, UPLOAD_HANDLER_KEY).ptr;
    nxd_fwbuffer* fwb= &ufo->fwbuffer;
    fclose((FILE *)fwb->fd);
    fwb->fd=0;
    //printf("on post_complete...\n");
    return NXWEB_OK;
}

nxweb_handler upload_file_handler={
    .on_request = upload_on_request,
    .on_post_data = upload_on_post_data,
    .on_post_data_complete = upload_on_post_data_complete,
    .flags = NXWEB_HANDLE_ANY};
