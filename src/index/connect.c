/* ai_connect.c
 * 
 * huangtao@antiy.com
 */

#include <pthread.h>
#include <curl/curl.h>
#include "def.h"
#include "connect.h"

static ADFS_RESULT upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len);
static ADFS_RESULT delete(CURL *curl, const char *url);


ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL)
        return res;

    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i)
    {
        if (pthread_mutex_trylock(pn->curl_mutex + i) == 0)
        {
            res = upload(pn->curl[i], url, fname, fdata, fdata_len);
            pthread_mutex_unlock(pn->curl_mutex + i);
            return res;
        }
    }

    pthread_mutex_t * pmutex = pn->curl_mutex + ADFS_NODE_CURL_NUM -1;
    pthread_mutex_lock(pmutex);
    res = upload(pn->curl[ADFS_NODE_CURL_NUM -1], url, fname, fdata, fdata_len);
    pthread_mutex_unlock(pmutex);
    return res;
}

ADFS_RESULT aic_delete(AINode *pn, const char *url)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL)
        return res;

    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i)
    {
        if (pthread_mutex_trylock(pn->curl_mutex + i) == 0)
        {
            res = delete(pn->curl[i], url);
            pthread_mutex_unlock(pn->curl_mutex + i);
            return res;
        }
    }

    pthread_mutex_t * pmutex = pn->curl_mutex + ADFS_NODE_CURL_NUM -1;
    pthread_mutex_lock(pmutex);
    res = delete(pn->curl[ADFS_NODE_CURL_NUM -1], url);
    pthread_mutex_unlock(pmutex);
    return res;
}


static size_t fun_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return nmemb;
}

static ADFS_RESULT upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    if (curl == NULL)
        return ADFS_ERROR;
    ADFS_RESULT adfs_res = ADFS_ERROR;

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastpost = NULL;
    curl_formadd(&formpost, &lastpost,
            CURLFORM_COPYNAME, "file",
            CURLFORM_BUFFER, fname,
            CURLFORM_BUFFERPTR, fdata,
            CURLFORM_BUFFERLENGTH, fdata_len,
            CURLFORM_END);

    static const char exp[] = "Expect:";
    struct curl_slist *headerlist = NULL;
    headerlist = curl_slist_append(headerlist, exp);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200)
            adfs_res = ADFS_OK;
    }

    curl_formfree(formpost);
    curl_slist_free_all(headerlist);
    return adfs_res;
}


static ADFS_RESULT delete(CURL *curl, const char *url)
{
    if (curl == NULL)
        return ADFS_ERROR;
    ADFS_RESULT adfs_res = ADFS_ERROR;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK)
    {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200)
            adfs_res = ADFS_OK;
    }
    return adfs_res;
}