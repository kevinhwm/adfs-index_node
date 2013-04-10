/* ai_connect.c
 * 
 * huangtao@antiy.com
 */

#include <curl/curl.h>


ADFS_RESULT aic_upload(const char *url, const char *fname, void *data, size_t len)
{
    ADFS_RESULT adfs_res = ADFS_ERROR;

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastpost = NULL;

    CURL *curl = curl_easy_init();
    if (curl)
    {
        curl_formadd(&formpost, &lastptr,
                CURLFORM_COPYNAME, "file",
                CURLFORM_BUFFER, fname,
                CURLFORM_BUFFERPTR, data,
                CURLFORM_BUFFERLENGTH, sizeof(data),
                CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        CURLcode res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            long res_code = 0;
            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res_code);
            if (res == CURLE_OK && res_code == 200)
            {
                adfs_res = ADFS_OK;
                printf("ok\n");
            }
        }
        curl_formfree(formpost);
    }

    curl_easy_cleanup(curl);
    return adfs_res;
}

