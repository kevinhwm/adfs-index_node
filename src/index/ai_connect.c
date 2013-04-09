/* ai_connect.c
 * 
 * huangtao@antiy.com
 */

#include <curl/curl.h>


ADFS_RESULT ai_save(const char *url, const char *fname, void *data, size_t len)
{
    CURL *curl = NULL;
    CURLcode res = 0;
    static const char exp[] = "Expect:";

    curl = curl_easy_init();

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastpost = NULL;
    struct curl_slist *headerlist = NULL;

    if (curl)
    {
        headerlist = curl_slist_append(headerlist, buf);
        curl_formadd(&formpost, &lastptr,
                CURLFORM_COPYNAME, "file",
                CURLFORM_BUFFER, fname,
                CURLFORM_BUFFERPTR, data,
                CURLFORM_BUFFERLENGTH, sizeof(data),
                CURLFORM_CONTENTHEADER, headerlist,
                CURLFORM_END);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, fun_write);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
        curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
            printf("ok\n");
        else
            printf("error\n");

        curl_slist_free_all(headerlist);
        curl_formfree(formpost);
    }

    curl_easy_cleanup(curl);
    return ADFS_OK;
}

ADFS_RESULT ai_get()
{
    return ADFS_OK;
}




