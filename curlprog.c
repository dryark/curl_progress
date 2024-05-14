#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <curl/curl.h>

// Define a structure to hold function pointers for the libcurl functions we need
struct curl_functions {
    CURL* (*easy_init)(void);
    CURLcode (*easy_setopt)(CURL *curl, CURLoption option, ...);
    CURLcode (*easy_perform)(CURL *curl);
    void (*easy_cleanup)(CURL *curl);
    const char* (*easy_strerror)(CURLcode);
    struct curl_slist* (*slist_append)(struct curl_slist *, const char *);
    void (*slist_free_all)(struct curl_slist *);
    const char* (*version)(void);
};

// Callback function for writing received data to a file
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp) {
    FILE *writehere = (FILE *)userp;
    return fwrite(buffer, size, nmemb, writehere);
}

// Progress callback function
int progress_callback(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow) {
    static double last_dl = 0;
    if (dlnow - last_dl > 20480) { // 20KB
        printf("%.0f\n", dlnow-last_dl);
        last_dl = dlnow;
    }
    return 0;
}

// Header callback function to print each header line
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    fwrite(buffer, size, nitems, (FILE *)userdata);
    return nitems * size;
}

int main() {
    void *libcurl;
    struct curl_functions curl;
    CURL *handle;
    CURLcode res;

    // Load the libcurl shared library
    libcurl = dlopen("/usr/lib/libcurl.dylib", RTLD_LAZY);
    if (!libcurl) {
        fprintf(stderr, "Error loading libcurl: %s\n", dlerror());
        return 1;
    }

    // Load function symbols
    curl.easy_init = dlsym(libcurl, "curl_easy_init");
    curl.easy_setopt = dlsym(libcurl, "curl_easy_setopt");
    curl.easy_perform = dlsym(libcurl, "curl_easy_perform");
    curl.easy_cleanup = dlsym(libcurl, "curl_easy_cleanup");
    curl.easy_strerror = dlsym(libcurl, "curl_easy_strerror");
    curl.slist_append = dlsym(libcurl, "curl_slist_append");
    curl.slist_free_all = dlsym(libcurl, "curl_slist_free_all");
    curl.version = dlsym(libcurl, "curl_version");

    if (!curl.easy_init || !curl.easy_setopt || !curl.easy_perform || !curl.easy_cleanup ||
        !curl.easy_strerror || !curl.slist_append || !curl.slist_free_all || !curl.version) {
        fprintf(stderr, "Error loading functions: %s\n", dlerror());
        dlclose(libcurl);
        return 1;
    }

    //const char* version = curl.version();
    //printf("Curl version: %s\n", version );
    //libcurl/8.4.0 SecureTransport (LibreSSL/3.3.6) zlib/1.2.11 nghttp2/1.51.0
    
    // Initialize a libcurl easy session
    handle = curl.easy_init();
    if (handle) {
        FILE *fp = fopen("openssl.tar.gz", "wb");
        if (!fp) {
            perror("File opening failed");
            return EXIT_FAILURE;
        }
        
        // Set URL
        curl.easy_setopt(handle, CURLOPT_URL, "https://ghcr.io/v2/homebrew/core/openssl/3/blobs/sha256:28be258776e175a8c29a19be5312b885574a98324d7b03c7ec12f2d7eadcbce1");
        // Set progress function and enable it
        curl.easy_setopt(handle, CURLOPT_NOPROGRESS, 0L);
        curl.easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);
        curl.easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, progress_callback);
        // Set headers
        struct curl_slist *headers = NULL;
        headers = curl.slist_append(headers, "User-Agent: Homebrew/4.2.20 (Macintosh; Intel Mac OS X 13.6.4) curl/8.4.0");
        headers = curl.slist_append(headers, "Accept-Language: en");
        headers = curl.slist_append(headers, "Authorization: Bearer QQ==");
        curl.easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
        // Set write function
        curl.easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_data);
        curl.easy_setopt(handle, CURLOPT_WRITEDATA, fp);
        
        //curl.easy_setopt(handle, CURLOPT_HEADERFUNCTION, header_callback);
        //curl.easy_setopt(handle, CURLOPT_HEADERDATA, stdout);

        // Perform the request
        res = curl.easy_perform(handle);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl.easy_strerror(res));
        }

        // Cleanup
        curl.easy_cleanup(handle);
        curl.slist_free_all(headers);
        fclose(fp);
    }

    dlclose(libcurl);
    return 0;
}