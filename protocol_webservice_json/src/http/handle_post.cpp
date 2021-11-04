/*
 *  handle_post.cpp
 *
 *  Created on: Dec 09, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Simple HTTP POST
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
//==============================================================================
// FUNCTIONS
//==============================================================================
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

bool post_json_mimepart(CaptureVehicle* capture, std::string SRV, struct curl_slist* httpHeaders) {
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

        // set destination
        curl_easy_setopt(curl, CURLOPT_URL, SRV.c_str());

        // specify we want to post
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // change Content-Type and remove Expect
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeaders);

        // passing FILE containing the POST data
        curl_easy_setopt(curl, CURLOPT_READDATA, (void*)json->get_file());

        // process response callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite_CallbackFunc_StdString);

        // process response argument
        LOG_INFO << "Sending POST request";
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);

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

        curl_easy_cleanup(curl);

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return result;
}

void post_capture(CaptureVehicleQueue& jsonQueue, nlohmann::json& config)
{
    const std::string SRV = config["remote_addr"].get<std::string>() + ":"
                          + std::to_string(config["remote_port"].get<int>())
                          + config["remote_api_url"].get<std::string>();

    curl_global_init(CURL_GLOBAL_ALL);

    // HTTP headers
    static struct curl_slist* httpHeaders = NULL;
    httpHeaders = curl_slist_append(httpHeaders, "Content-Type: application/json");
    httpHeaders = curl_slist_append(httpHeaders, "Expect:");

    LOG_INFO << "Start HTTP Post process";

    while (true) {
        // local init
        CaptureVehicle* capture = jsonQueue.pop();

        while(!post_json_mimepart(capture, SRV, httpHeaders)) {
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

curl_socket_t opensocket(void*, curlsocktype, struct curl_sockaddr*)
{
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
