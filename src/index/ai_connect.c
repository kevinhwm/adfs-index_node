/* ai_connect.c
 * 
 * huangtao@antiy.com
 */

#include <curl/curl.h>
#include "ai.h"

static ADFS_RESULT upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len);


ADFS_RESULT aic_upload(AINode *pn, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    ADFS_RESULT res = ADFS_ERROR;
    if (pn == NULL)
        return res;

    DBG_PRINTS("aic-upload 10\n");
    for (int i=0; i<ADFS_NODE_CURL_NUM; ++i)
    {
        if (pthread_mutex_trylock(pn->curl_mutex + i) == 0)
        {
#ifdef DEBUG
            static unsigned long a = 1000;
            DBG_PRINTIN(++a);
#endif // DEBUG
            res = upload(pn->curl[i], url, fname, fdata, fdata_len);
            DBG_PRINTS("do upload finish\n");
            pthread_mutex_unlock(pn->curl_mutex + i);
            DBG_PRINTS("mutex unlock finish\n");
            return res;
        }
    }

    DBG_PRINTS("aic-upload 20\n");
    pthread_mutex_t * pmutex = pn->curl_mutex + ADFS_NODE_CURL_NUM -1;
    pthread_mutex_lock(pmutex);
    res = upload(pn->curl[ADFS_NODE_CURL_NUM -1], url, fname, fdata, fdata_len);
    pthread_mutex_unlock(pmutex);
    return res;
}


static size_t upload_write(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    return nmemb;
}

static ADFS_RESULT upload(CURL *curl, const char *url, const char *fname, void *fdata, size_t fdata_len)
{
    DBG_PRINTS("10\n");

    if (curl == NULL)
        return ADFS_ERROR;

    DBG_PRINTS("20\n");
    ADFS_RESULT adfs_res = ADFS_ERROR;
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastpost = NULL;

    DBG_PRINTS("30\n");
    curl_formadd(&formpost, &lastpost,
            CURLFORM_COPYNAME, "file",
            CURLFORM_BUFFER, fname,
            CURLFORM_BUFFERPTR, fdata,
            CURLFORM_BUFFERLENGTH, fdata_len,
            CURLFORM_END);

    DBG_PRINTS("40\n");
    DBG_PRINTPN(curl);
    DBG_PRINTSN(url);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    DBG_PRINTS("41\n");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, upload_write);
    DBG_PRINTS("42\n");
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    DBG_PRINTS("50\n");
    CURLcode res = curl_easy_perform(curl);

    DBG_PRINTS("60\n");
    if (res == CURLE_OK)
    {
        long res_code = 0;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
        if (res == CURLE_OK && res_code == 200)
        {
            adfs_res = ADFS_OK;
            DBG_PRINTS("curl upload OK\n");
        }
    }
    curl_formfree(formpost);
    return adfs_res;
}

