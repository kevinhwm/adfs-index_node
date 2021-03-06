/* http_upload.c
 *
 * kevinhwm@gmail.com
 */

#include <nxweb/nxweb.h>
#include <kclangc.h>
#include "../multipart_parser.h"
#include "manager.h"

static const char upload_handler_key; 
#define UPLOAD_HANDLER_KEY ((nxe_data)&upload_handler_key)

typedef struct _upload_file_object
{
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


static int on_post_header_field(multipart_parser *mp_obj, const char *at, size_t length )
{
    // nothing to do
    return 0;
}

static int on_post_header_value( multipart_parser *mp_obj, const char *at, size_t length )
{
    char buff[1024] = {0};
    strncpy( buff, at, length );

    upload_file_object *pufo = multipart_parser_get_data( mp_obj );
    char *pfname = strstr( buff, "filename=\"" );
    if( pfname == NULL ) {
        if( strstr( buff, "name=\"upname\"" ) ) {
            pufo->filename_ready_to_receive = 1;
            pufo->file_ready_to_receive = 0;
            return 0;
        }
        else 
            return 0;
    }

    pfname += 10;
    char *pfname_end = strstr( pfname+1, "\"" );
    if (pfname_end == NULL) {memset(pufo->filename, 0, sizeof(pufo->filename));}
    else {strncpy( pufo->filename, pfname, pfname_end - pfname );}
    pufo->file_ready_to_receive = 1;
    return 0;
}

static int on_post_body( multipart_parser *mp_obj, const char *at, size_t length )
{
    upload_file_object *pufo = multipart_parser_get_data( mp_obj );
    if ( pufo->filename_ready_to_receive == 1 ) {
        strncpy( pufo->filename, at, length );
        pufo->filename_ready_to_receive = 0;
        pufo->file_ready_to_receive = 1;
        return 0;
    }
    if ( pufo->file_ready_to_receive ) {
        if ( pufo->ffilemem == NULL ) {return 0;}
        if ( pufo->ffilemem ) {fwrite( at, 1, length, pufo->ffilemem);}
    }
    return 0;
}

int on_post_finished(multipart_parser * mp_obj)
{
    upload_file_object *pufo = multipart_parser_get_data( mp_obj );
    if ( pufo->ffilemem ) {
        fclose( pufo->ffilemem );
        pufo->file_complete = 1;
        pufo->ffilemem=NULL;
    }
    return 0;
}

static nxweb_result upload_on_request(
        nxweb_http_server_connection* conn, 
        nxweb_http_request* req, 
        nxweb_http_response* resp)
{ 
    nxweb_set_response_content_type(resp, "text/html");
    nxweb_set_response_charset(resp, "utf-8");
    nxweb_parse_request_parameters(req, 0);
    const char *name_space = nx_simple_map_get_nocase(req->parameters, "namespace");

    int res = 0;
    char fname[_DFS_MAX_LEN] = {0};
    upload_file_object *ufo = nxweb_get_request_data(req, UPLOAD_HANDLER_KEY).ptr;
    nxd_fwbuffer* fwb = &ufo->fwbuffer;
    if (fwb) {
	ufo->parser_settings.on_header_field = on_post_header_field;
	ufo->parser_settings.on_header_value = on_post_header_value;
	ufo->parser_settings.on_part_data = on_post_body;
	ufo->parser_settings.on_body_end = on_post_finished;
	ufo->parser = multipart_parser_init( ufo->post_boundary, &ufo->parser_settings );

	multipart_parser_set_data( ufo->parser, ufo );
	ufo->ffilemem = open_memstream( (char **)&ufo->file_ptr, &ufo->file_len );
	multipart_parser_execute( ufo->parser, ufo->postdata_ptr, ufo->postdata_len );
	multipart_parser_free( ufo->parser );

	if ( strlen(ufo->filename) > 0 && ufo->file_complete ) {
	    strncpy(fname, req->path_info, sizeof(fname));
	    nxweb_url_decode(fname, NULL);
	    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
	    if (get_filename_from_url(fname, pattern) < 0) {
		nxweb_send_http_error(resp, 400, "Failed. Check file name.");
		res = -1;
	    }
	    else if (GNm_save(name_space, fname, strlen(fname), ufo->file_ptr, ufo->file_len) < 0) {
		nxweb_send_http_error(resp, 400, "Failed. Can not save.");
		res = -1;
	    }
	    else  {
		nxweb_response_printf(resp, "OK.\n");
		res = 1;
	    }
	}
	else {
	    nxweb_send_http_error(resp, 400, "Failed. Check file name and name length.");
	    res = -1;
	}

	if (ufo->file_ptr) { free( ufo->file_ptr ); ufo->file_ptr = NULL; }
    }

    char msg[1024] = {0};
    if (res < 0) {
	snprintf(msg, sizeof(msg), "[%s:%s]->error.[%s]", name_space, fname, conn->remote_addr);
	log_out("upload", msg, LOG_LEVEL_INFO);
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }
    else if (res > 0) {snprintf(msg, sizeof(msg), "[%s:%s]->ok.[%s]", name_space, fname, conn->remote_addr);}
    else {snprintf(msg, sizeof(msg), "[%s:%s]->request.[%s]", name_space, fname, conn->remote_addr);}

    log_out("upload", msg, LOG_LEVEL_INFO);
    nxweb_response_printf(resp, ""
	    "<html><head><title>Upload Module</title></head><body>\n"
	    "<form method='post' enctype='multipart/form-data'>\n"
	    "File(s) to upload: "
	    "<input type='file' multiple name='uploadedfile' />\n"
	    "<input type='submit' value='Upload' />\n"
	    "</form></body></html>");
    return NXWEB_OK;
}

static void upload_request_data_finalize(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp, 
	nxe_data data) 
{
    upload_file_object *ufo = data.ptr;
    nxd_fwbuffer* fwb= &ufo->fwbuffer;
    if (fwb && fwb->fd) { fclose(fwb->fd); fwb->fd=0; }
    if (ufo->postdata_ptr) { free( ufo->postdata_ptr ); ufo->postdata_ptr = NULL; }
}

static nxweb_result upload_on_post_data(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    upload_file_object* ufo = nxb_alloc_obj(req->nxb, sizeof(upload_file_object));
    memset( ufo, 0, sizeof( upload_file_object ) );
    nxd_fwbuffer* fwb = &ufo->fwbuffer;
    nxweb_set_request_data(req, UPLOAD_HANDLER_KEY, (nxe_data)(void*)ufo, upload_request_data_finalize);
    sscanf(req->content_type, "%*[^=]%*1s%s", ufo->post_boundary+2);
    ufo->post_boundary[0] = '-';
    ufo->post_boundary[1] = '-';
    ufo->fpostmem = open_memstream( (char **)&ufo->postdata_ptr, &ufo->postdata_len );
    nxd_fwbuffer_init(fwb, ufo->fpostmem, req->content_length);
    conn->hsp.cls->connect_request_body_out(&conn->hsp, &fwb->data_in);
    conn->hsp.cls->start_receiving_request_body(&conn->hsp);
    return NXWEB_OK;
}

static nxweb_result upload_on_post_data_complete(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    // It is not strictly necessary to close the file here
    // as we are closing it anyway in request data finalizer.
    // Releasing resources in finalizer is the proper way of doing this
    // as any other callbacks might not be invoked under error conditions.
    upload_file_object* ufo = nxweb_get_request_data(req, UPLOAD_HANDLER_KEY).ptr;
    nxd_fwbuffer* fwb= &ufo->fwbuffer;
    fclose(fwb->fd);
    fwb->fd=0;
    return NXWEB_OK;
}


nxweb_handler upload_file_handler={
    .on_request = upload_on_request,
    .on_post_data = upload_on_post_data,
    .on_post_data_complete = upload_on_post_data_complete,
    .flags = NXWEB_HANDLE_ANY
};

