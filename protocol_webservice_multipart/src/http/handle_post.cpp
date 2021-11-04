/*
 *  handle_post.cpp
 *
 *  Created on: Jul 29, 2021
 *      Author: Alexandre Krzyzanovski - Pumatronix
 *
 *  HTTP POST Multipart
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/file_handler.h"
#include "helpers/log.h"
#include "http/handle_post.h"

#include "camera/jlib/jfs/filesystem.h"

#include <chrono>
#include <thread>
#include <arpa/inet.h>

size_t CurlWrite_CallbackFunc_StdString(void *contents, size_t size, size_t nmemb, std::string *s) {
    size_t newLength = size*nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch(std::bad_alloc &e) {
        //handle memory problem
        return 0;
    }
    return newLength;
}

bool post_multipart_mimepart(CaptureVehicle* capture, std::string SRV, struct curl_slist* httpHeaders) {
    bool result = false;
    int id = 0;
    auto plates = capture->get_plates();
    for ( const auto& json : capture->get_jsons() ) {
        std::string plate = plates[id];
        id++;
        if(!fs::exists(json->get_path())) {
            LOG_ERR << "JSON " << json->get_path() << " was removed!";
            return true;
        }
        curl_mime *form = NULL;
        curl_mimepart *field = NULL;

        CURLcode CurlResult;
        long CurlStatus;
        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG_ERR << "Failed to initialize libcurl";
            break;
        }

        std::string response;
        // no progress meter
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        // use custom made socket
        curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, opensocket);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);			//Connect timeout in secs
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);				//Maximum time the request is allowed to take in secs
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        // set destination
        curl_easy_setopt(curl, CURLOPT_URL, SRV.c_str());
        // specify we want to post
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        // change Content-Type and remove Expect
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaders);
        /* Create the form */
        form = curl_mime_init(curl);
        /* Fill in the file upload field */
        field = curl_mime_addpart(form);
        curl_mime_name(field, "passage");
        curl_mime_type(field, "application/json");
        curl_mime_filedata(field, json->get_path().c_str());
        curl_mime_filename(field, "passagebody.json");;
        // LOG_INFO << "curl_mime_filedata(passage," << json->get_path() << ")";

        int index = 0;
        std::string fimg;
        for ( const auto& image : capture->get_images() ) {
            if(!fs::exists(image->get_path())) {
                fimg = image->get_path();
                continue;
            }
            std::string fieldname = "images["+std::to_string(index)+"]";
            std::string filename = "passagefoto"+std::to_string(index)+".jpg";
            /* Fill in the file upload field */
            field = curl_mime_addpart(form);
            curl_mime_type(field, "image/jpeg");
            curl_mime_name(field, fieldname.c_str());
            curl_mime_filedata(field, image->get_path().c_str());
            curl_mime_filename(field, filename.c_str());;
            // LOG_INFO << "curl_mime_filedata("<< fieldname <<"," << image->get_path() << ")";
            index++;
        }
        if(index == 0) {
            // no image was add... ERROR!!
            curl_mime_free(form);
            /* always cleanup */
            curl_easy_cleanup(curl);
            LOG_ERR << "Image " << fimg << " was removed!";
            return true;
        }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        /* Perform the request, res will get the return code */
        CurlResult = curl_easy_perform(curl);
        
        /* Check for errors */
        if(CurlResult != CURLE_OK) {
            LOG_ERR << "POST " << plate << " failed with error: " << curl_easy_strerror(CurlResult);
        } else {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &CurlStatus);
            if((CurlStatus-(CurlStatus % 100)) == 200) {
                LOG_INFO << "POST " << plate << " OK [" << CurlStatus << "] " << response;
                result = true;
            } else {
                LOG_ERR << "POST " << plate << " HTTP ERROR [" << CurlStatus << "] " << response;
            }
        }
        /* then cleanup the form */
        curl_mime_free(form);
        /* always cleanup */
        curl_easy_cleanup(curl);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return result;
}

//==============================================================================
// FUNCTIONS
//==============================================================================
void post_capture(CaptureVehicleQueue& jsonQueue, nlohmann::json& config)
{
    const std::string SRV = config["remote_addr"].get<std::string>() + ":"
                          + std::to_string(config["remote_port"].get<int>())
                          + config["remote_api_url"].get<std::string>();

    curl_global_init(CURL_GLOBAL_ALL);

    // HTTP headers
    static struct curl_slist* httpHeaders = NULL;
    
    httpHeaders = curl_slist_append(httpHeaders, "cache-control: no-cache");
    httpHeaders = curl_slist_append(httpHeaders, "Content-Type: multipart/form-data");    
    httpHeaders = curl_slist_append(httpHeaders, "Expect:");

    LOG_INFO << "Start HTTP Post process";

    while (true) {
        // local init
        CaptureVehicle* capture = jsonQueue.pop();

        while(!post_multipart_mimepart(capture, SRV, httpHeaders)) {
            // error to send... retry after 5 seconds
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        delete capture;
        capture = NULL;
    }
    // global cleanup
    curl_slist_free_all(httpHeaders);
    curl_global_cleanup();
    LOG_ERR << "Terminating due to a failure";
    exit(EXIT_FAILURE);
}

curl_socket_t opensocket(void*, curlsocktype, struct curl_sockaddr*) {
    // set socket
    curl_socket_t sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // validate creation
    if (sockfd == CURL_SOCKET_BAD) {
        LOG_ERR << "Failed to create socket";
        exit(EXIT_FAILURE);
    }

    // closes immediately on close()
    struct linger lin = { 1, 0 };
    size_t lin_sz = sizeof(struct linger);

    if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (const char*)&lin, lin_sz)) {
        LOG_ERR << "Failed to set socket options";
        exit(EXIT_FAILURE);
    }

    // bind it to <all interfaces>:<LOCAL_PORT>
    struct sockaddr_in localAddr;

    localAddr.sin_family = AF_INET;
    localAddr.sin_addr.s_addr = INADDR_ANY;
    localAddr.sin_port = htons(9999);

    if (bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr))) {
        LOG_ERR << "Failed to bind local address";
        exit(EXIT_FAILURE);
    }

    return sockfd;
}
