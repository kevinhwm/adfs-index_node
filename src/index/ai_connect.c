/* ai_connect.c
 * 
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <pthread.h>
#include <curl/curl.h>
#include "../include/adfs.h"
#include "ai_zone.h"

enum FLAG_CONNECTION
{
    FLAG_UPLOAD 	= 1,
    FLAG_CONNECT
};

static ADFS_RESULT c_upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len);
static ADFS_RESULT c_connect(CURL *curl, const char *url);
static void c_rebuild(AINode *pn, int pos);


ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL) {return res;}
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        if (pthread_mutex_trylock(pn->conn[i].mutex) == 0) {
	    if (pn->conn[i].flag!=0 && pn->conn[i].flag!=FLAG_UPLOAD) {c_rebuild(pn, i);}
            res = c_upload(pn->conn[i].curl, url, fname, fdata, fdata_len);
	    pn->conn[i].flag = FLAG_UPLOAD;
            pthread_mutex_unlock(pn->conn[i].mutex);
            return res;
        }
    }

    int pos_last = ADFS_NODE_CURL_NUM -1;
    pthread_mutex_t * pmutex = pn->conn[pos_last].mutex;

    pthread_mutex_lock(pmutex);
    if (pn->conn[pos_last].flag != 0 && pn->conn[pos_last].flag != FLAG_UPLOAD) {c_rebuild(pn, pos_last);}
    res = c_upload(pn->conn[pos_last].curl, url, fname, fdata, fdata_len);
    pn->conn[pos_last].flag = FLAG_UPLOAD;
    pthread_mutex_unlock(pmutex);
    return res;
}

ADFS_RESULT aic_connect(AINode *pn, const char *url)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL) {return res;}
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        if (pthread_mutex_trylock(pn->conn[i].mutex) == 0) {
	    if (pn->conn[i].flag != 0 && pn->conn[i].flag != FLAG_CONNECT) {c_rebuild(pn, i);}
            res = c_connect(pn->conn[i].curl, url);
	    pn->conn[i].flag = FLAG_CONNECT;
            pthread_mutex_unlock(pn->conn[i].mutex);
            return res;
        }
    }

    int pos_last = ADFS_NODE_CURL_NUM -1;
    pthread_mutex_t * pmutex = pn->conn[pos_last].mutex;

    pthread_mutex_lock(pmutex);
    if (pn->conn[pos_last].flag != 0 && pn->conn[pos_last].flag != FLAG_CONNECT) {c_rebuild(pn, pos_last);}
    res = c_connect(pn->conn[pos_last].curl, url);
    pn->conn[pos_last].flag = FLAG_CONNECT;
    pthread_mutex_unlock(pmutex);
    return res;
}

//////////////////////////////////////////////////////////////////////////////////////
// private
static size_t fun_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return nmemb;
}

static ADFS_RESULT c_upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    ADFS_RESULT adfs_res = ADFS_ERROR;
    if (curl == NULL) {return ADFS_ERROR;}
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastpost = NULL;
    curl_formadd(&formpost, &lastpost,
            CURLFORM_COPYNAME, "file",
            CURLFORM_BUFFER, fname,
            CURLFORM_BUFFERPTR, fdata,
            CURLFORM_BUFFERLENGTH, fdata_len,
            CURLFORM_END);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 7);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200) {adfs_res = ADFS_OK;}
    }
    curl_formfree(formpost);
    return adfs_res;
}

static ADFS_RESULT c_connect(CURL *curl, const char *url)
{
    ADFS_RESULT adfs_res = ADFS_ERROR;
    if (curl == NULL) {return ADFS_ERROR;}
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200) {adfs_res = ADFS_OK;}
    }
    return adfs_res;
}

static void c_rebuild(AINode *pn, int pos)
{
    if (pn->conn[pos].curl) {curl_easy_cleanup(pn->conn[pos].curl);}
    pn->conn[pos].curl = curl_easy_init();
}

