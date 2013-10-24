/* if_download.c
 *
 * kevinhwm@gmail.com
 */

#include "nxweb/nxweb.h"
#include "kclangc.h"
#include "an_manager.h"

static const char download_handler_key;
#define DOWNLOAD_HANDLER_KEY ((nxe_data)&download_handler_key)

typedef struct SHARE_DATA{
    void * data_ptr;
}SHARE_DATA;

static void download_request_data_finalize(
	nxweb_http_server_connection *conn,
	nxweb_http_request *req,
	nxweb_http_response *resp,
	nxe_data data)
{
    SHARE_DATA * d = data.ptr;
    if(d->data_ptr) { kcfree(d->data_ptr); d->data_ptr = NULL; }
}

static nxweb_result download_on_request(
	nxweb_http_server_connection* conn, 
	nxweb_http_request* req, 
	nxweb_http_response* resp) 
{
    if (strlen(req->path_info) >= ADFS_MAX_LEN) {
	nxweb_send_http_error(resp, 400, "Failed. File name is too long.");
	resp->keep_alive = 0;
	return NXWEB_ERROR;
    }
    nxweb_parse_request_parameters(req, 0);
    const char *name_space = nx_simple_map_get_nocase(req->parameters, "namespace");
    char fname[ADFS_MAX_LEN] = {0};
    strncpy(fname, req->path_info, sizeof(fname));
    nxweb_url_decode(fname, NULL);
    char *pattern = "^(/[\\w./]{1,512}){1}(\\?[\\w%&=]+)?$";
    if (get_filename_from_url(fname, pattern) < 0) {
	nxweb_send_http_error(resp, 400, "Failed. Check file name.");
	return NXWEB_ERROR;
    }

    char msg[1024] = {0};
    void *pfile_data = NULL;
    size_t file_size = 0;
    anm_get(name_space, fname, &pfile_data, &file_size);    // query db
    if (pfile_data == NULL) {
	nxweb_send_http_error(resp, 404, "Failed. No file.");
	snprintf(msg, sizeof(msg), "[%s:%s]->error.[%s]", name_space, fname, conn->remote_addr);
	log_out("download", msg, LOG_LEVEL_INFO);
	return NXWEB_ERROR;
    }
    else {
	SHARE_DATA *ptmp = nxb_alloc_obj(req->nxb, sizeof(SHARE_DATA));
	nxweb_set_request_data(req, DOWNLOAD_HANDLER_KEY, (nxe_data)(void *)ptmp, download_request_data_finalize);
	ptmp->data_ptr = pfile_data;
	char *file_name = fname;
	char *tmp = NULL;
	while ( (tmp = strstr(file_name, "/")) ) {file_name = tmp + 1;}

	char resp_name[ADFS_FILENAME_LEN] = {0};
	snprintf( resp_name, sizeof(resp_name), "attachment; filename=%.*s", (int)(strlen(file_name)-ADFS_UUID_LEN), file_name);
	nxweb_add_response_header(resp, "Content-disposition", resp_name );
	nxweb_send_data( resp, pfile_data, file_size, "application/octet-stream" );
	snprintf(msg, sizeof(msg), "[%s:%s]->ok.[%s]", name_space, fname, conn->remote_addr);
	log_out("download", msg, LOG_LEVEL_INFO);
    }
    return NXWEB_OK;
}

nxweb_handler download_handler={
    .on_request = download_on_request,
    .flags = NXWEB_PARSE_PARAMETERS
};

