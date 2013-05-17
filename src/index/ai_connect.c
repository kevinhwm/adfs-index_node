/* ai_connect.c
 * 
 * huangtao@antiy.com
 * Antiy Labs. Basic Platform R & D Center.
 */

#include <pthread.h>
#include <curl/curl.h>
#include "adfs.h"
#include "ai_zone.h"

enum FLAG_CONNECTION
{
    FLAG_UPLOAD 	= 1,
    FLAG_DOWNLOAD,
    FLAG_REMOVE,
    FLAG_STATUS
};

static ADFS_RESULT c_upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len);
static ADFS_RESULT c_erase(CURL *curl, const char *url);
static ADFS_RESULT c_status(CURL *curl, const char *url);
static void c_reconnect(AINode *pn, int pos);


ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL)
        return res;
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        if (pthread_mutex_trylock(pn->curl_mutex + i) == 0) {
	    if (pn->flag[i] != 0 && pn->flag[i] != FLAG_UPLOAD)
		c_reconnect(pn, i);
            res = c_upload(pn->curl[i], url, fname, fdata, fdata_len);
	    pn->flag[i] = FLAG_UPLOAD;
            pthread_mutex_unlock(pn->curl_mutex + i);
            return res;
        }
    }
    pthread_mutex_t * pmutex = pn->curl_mutex + ADFS_NODE_CURL_NUM -1;
    pthread_mutex_lock(pmutex);
    if (pn->flag[ADFS_NODE_CURL_NUM-1] != 0 && pn->flag[ADFS_NODE_CURL_NUM-1] != FLAG_UPLOAD)
	c_reconnect(pn, ADFS_NODE_CURL_NUM-1);
    res = c_upload(pn->curl[ADFS_NODE_CURL_NUM -1], url, fname, fdata, fdata_len);
    pn->flag[ADFS_NODE_CURL_NUM -1] = FLAG_UPLOAD;
    pthread_mutex_unlock(pmutex);
    return res;
}

ADFS_RESULT aic_erase(AINode *pn, const char *url)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL)
        return res;
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        if (pthread_mutex_trylock(pn->curl_mutex + i) == 0) {
	    if (pn->flag[i] != 0 && pn->flag[i] != FLAG_REMOVE)
		c_reconnect(pn, i);
            res = c_erase(pn->curl[i], url);
	    pn->flag[i] = FLAG_REMOVE;
            pthread_mutex_unlock(pn->curl_mutex + i);
            return res;
        }
    }
    pthread_mutex_t * pmutex = pn->curl_mutex + ADFS_NODE_CURL_NUM -1;
    pthread_mutex_lock(pmutex);
    if (pn->flag[ADFS_NODE_CURL_NUM-1] != 0 && pn->flag[ADFS_NODE_CURL_NUM-1] != FLAG_REMOVE)
	c_reconnect(pn, ADFS_NODE_CURL_NUM-1);
    res = c_erase(pn->curl[ADFS_NODE_CURL_NUM -1], url);
    pn->flag[ADFS_NODE_CURL_NUM -1] = FLAG_REMOVE;
    pthread_mutex_unlock(pmutex);
    return res;
}

ADFS_RESULT aic_status(AINode *pn, const char *url)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL)
        return res;
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i) {
        if (pthread_mutex_trylock(pn->curl_mutex + i) == 0) {
	    if (pn->flag[i] != 0 && pn->flag[i] != FLAG_STATUS)
		c_reconnect(pn, i);
            res = c_status(pn->curl[i], url);
	    pn->flag[i] = FLAG_STATUS;
            pthread_mutex_unlock(pn->curl_mutex + i);
            return res;
        }
    }
    pthread_mutex_t * pmutex = pn->curl_mutex + ADFS_NODE_CURL_NUM -1;
    pthread_mutex_lock(pmutex);
    if (pn->flag[ADFS_NODE_CURL_NUM-1] != 0 && pn->flag[ADFS_NODE_CURL_NUM-1] != FLAG_STATUS)
	c_reconnect(pn, ADFS_NODE_CURL_NUM-1);
    res = c_status(pn->curl[ADFS_NODE_CURL_NUM -1], url);
    pn->flag[ADFS_NODE_CURL_NUM -1] = FLAG_STATUS;
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
    if (curl == NULL)
        return ADFS_ERROR;
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
    DBG_PRINTPN(curl);
    DBG_PRINTSN(url);
    DBG_PRINTUN(res);
    if (res == CURLE_OK) {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200)
            adfs_res = ADFS_OK;
    }
    curl_formfree(formpost);
    return adfs_res;
}

static ADFS_RESULT c_erase(CURL *curl, const char *url)
{
    ADFS_RESULT adfs_res = ADFS_ERROR;
    if (curl == NULL)
        return ADFS_ERROR;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 7);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200)
            adfs_res = ADFS_OK;
    }
    return adfs_res;
}

static ADFS_RESULT c_status(CURL *curl, const char *url)
{
    ADFS_RESULT adfs_res = ADFS_ERROR;
    if (curl == NULL)
        return ADFS_ERROR;
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200)
            adfs_res = ADFS_OK;
    }
    return adfs_res;
}


static void c_reconnect(AINode *pn, int pos)
{
    if (pn->curl[pos])
	curl_easy_cleanup(pn->curl[pos]);
    pn->curl[pos] = curl_easy_init();
}

