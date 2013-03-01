#include "curl_upload.h"


int upkey(char *url, char *name, char *data)
{
    //  curl_global_init(CURL_GLOBAL_ALL);
    CURL * handle = curl_easy_init();
    CURLcode res;
    int succ = 0;
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    //  struct curl_slist *headers = NULL;
    if(handle)
    {
        //	headers = curl_slist_append( headers, "Expect:" );
        curl_formadd(&post, &last,
                CURLFORM_COPYNAME, "upname",
                CURLFORM_COPYCONTENTS, name,
                CURLFORM_END
                );

        curl_formadd(&post, &last,
                CURLFORM_COPYNAME, "data",
                CURLFORM_COPYCONTENTS, data,
                CURLFORM_END
                );
        curl_easy_setopt(handle, CURLOPT_URL, url);
        //	curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(handle, CURLOPT_HTTPPOST, post);
        curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, CURL_MAX_WRITE_SIZE + 32 );
        res = curl_easy_perform(handle);
        long http_code = 0;
        curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code==200)
        {
            succ=1;
            printf("%s", "\n");
        }
        else
        {
            printf("%s\n", "failed");
        }

        curl_easy_cleanup(handle);
        curl_formfree(post);
    }
    else
    {

        curl_easy_cleanup(handle);
        //     curl_global_cleanup();
        printf("%s\n", "handle failed");
    }
    return succ;
}

int upload(char *url, char* record, char*data, long record_length)
{
    int succ = 0;
    curl_global_init(CURL_GLOBAL_ALL);
    CURL * handle = curl_easy_init();
    CURLcode res;
    struct curl_httppost *post = NULL;
    struct curl_httppost *last = NULL;
    struct curl_slist *headers = NULL;

    if(handle)
    {
        struct curl_slist *form_headers = NULL;
        char header_info[128] = {0};
        struct curl_slist * headers = NULL;
        sprintf( header_info, "Content-Length: %ld", record_length );
        form_headers = curl_slist_append( form_headers, header_info );
        headers = curl_slist_append( headers, "Expect:" );
        curl_formadd(&post, &last,
                CURLFORM_COPYNAME, "myfile",
                CURLFORM_BUFFER, record,
                CURLFORM_BUFFERPTR, data,
                CURLFORM_BUFFERLENGTH, record_length,
                CURLFORM_CONTENTHEADER, form_headers,
                CURLFORM_END
                );

        curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(handle, CURLOPT_URL, url);
        curl_easy_setopt(handle, CURLOPT_HTTPPOST, post);
        curl_easy_setopt(handle, CURLOPT_BUFFERSIZE, CURL_MAX_WRITE_SIZE + 32 );
        res = curl_easy_perform(handle);
        long http_code = 0;
        curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code==200)
        {
            succ=1;
            printf("%s", "\n");
        }
        else
            printf("%s\n", "failed");

        curl_formfree(post);

        curl_slist_free_all (form_headers );
        curl_slist_free_all (headers );
    }
    else
        printf("%s\n", "handler failed");

    curl_easy_cleanup(handle);
    curl_global_cleanup();
    return succ;
}

int delete_key(char *url)
{
    CURL * handle = curl_easy_init();
    CURLcode res;
    int succ = 0;
    if(handle)
    {
        curl_easy_setopt(handle, CURLOPT_URL, url);
        res = curl_easy_perform(handle);
        long http_code = 0;
        curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &http_code);

        if (http_code == 200)
        {
            succ=1;
            printf("%s", "\n");
        }
        else
            printf("%s\n", "failed");

        curl_easy_cleanup(handle);
    }
    else
        curl_easy_cleanup(handle);
    return succ;
}


