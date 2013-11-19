/* connect.c
 * 
 * kevinhwm@gmail.com
 */

#include <pthread.h>
#include <curl/curl.h>
#include "../def.h"
#include "zone.h"

static int c_upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len);
static int c_connect(CURL *curl, const char *url);
static void c_rebuild(CINode *pn, int pos);


int GIc_upload(CINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    int res = -1;
    if (pn == NULL) {return res;}
    for (int i=0; i<_DFS_NODE_CURL_NUM; ++i) {
	if (pthread_mutex_trylock(pn->conn[i].mutex) == 0) {
	    if (pn->conn[i].flag!=FLAG_UPLOAD) {c_rebuild(pn, i);}
	    res = c_upload(pn->conn[i].curl, url, fname, fdata, fdata_len);
	    pn->conn[i].flag = FLAG_UPLOAD;
	    pthread_mutex_unlock(pn->conn[i].mutex);
	    return res;
	}
    }

    int pos_last = _DFS_NODE_CURL_NUM -1;
    pthread_mutex_t * pmutex = pn->conn[pos_last].mutex;

    pthread_mutex_lock(pmutex);
    if (pn->conn[pos_last].flag != FLAG_UPLOAD) {c_rebuild(pn, pos_last);}
    res = c_upload(pn->conn[pos_last].curl, url, fname, fdata, fdata_len);
    pn->conn[pos_last].flag = FLAG_UPLOAD;
    pthread_mutex_unlock(pmutex);
    return res;
}

int GIc_connect(CINode *pn, const char *url, FLAG_CONNECTION flag)
{
    int res = -1;
    if (pn == NULL) {return res;}
    for (int i=0; i<_DFS_NODE_CURL_NUM; ++i) {
	if (pthread_mutex_trylock(pn->conn[i].mutex) == 0) {
	    if (pn->conn[i].flag != flag) {c_rebuild(pn, i);}
	    res = c_connect(pn->conn[i].curl, url);
	    pn->conn[i].flag = flag;
	    pthread_mutex_unlock(pn->conn[i].mutex);
	    return res;
	}
    }

    int pos_last = _DFS_NODE_CURL_NUM -1;
    pthread_mutex_t * pmutex = pn->conn[pos_last].mutex;

    pthread_mutex_lock(pmutex);
    if (pn->conn[pos_last].flag != flag) {c_rebuild(pn, pos_last);}
    res = c_connect(pn->conn[pos_last].curl, url);
    pn->conn[pos_last].flag = flag;
    pthread_mutex_unlock(pmutex);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////
// private
static size_t fun_write(char *ptr, size_t size, size_t nmemb, void *userdata) { return nmemb; }

static int c_upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    int _dfs_res = -1;
    if (curl == NULL) {return -1;}
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastpost = NULL;
    curl_formadd(&formpost, &lastpost,
	    CURLFORM_COPYNAME, "file",
	    CURLFORM_BUFFER, fname,
	    CURLFORM_BUFFERPTR, fdata,
	    CURLFORM_BUFFERLENGTH, fdata_len,
	    CURLFORM_END);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
	long res_code = 0;
	res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
	if (res == CURLE_OK && res_code == 200) { _dfs_res = 0; }
    }
    curl_formfree(formpost);
    return _dfs_res;
}

static int c_connect(CURL *curl, const char *url)
{
    int _dfs_res = -1;
    if (curl == NULL) { return -1; }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
	long res_code = 0;
	res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
	if (res == CURLE_OK && res_code == 200) {_dfs_res = 0;}
    }
    return _dfs_res;
}

static void c_rebuild(CINode *pn, int pos)
{
    if (pn->conn[pos].curl) {curl_easy_cleanup(pn->conn[pos].curl);}
    pn->conn[pos].curl = curl_easy_init();
}

