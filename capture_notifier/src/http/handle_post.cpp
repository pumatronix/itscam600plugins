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
#include "config/config_parser.h"
#include "helpers/file_handler.h"
#include "helpers/log.h"
#include "http/handle_post.h"

#include <arpa/inet.h>

//==============================================================================
// FUNCTIONS
//==============================================================================
void post_capture(FilenameQueue& jsonQueue)
{
    auto config = Config::get_config();
    const std::string SRV = config["remote_addr"].get<std::string>() + ":"
                          + std::to_string(config["remote_port"].get<int>())
                          + config["remote_api_url"].get<std::string>();

    curl_global_init(CURL_GLOBAL_ALL);

    // HTTP headers
    static struct curl_slist* httpHeaders = NULL;
    httpHeaders = curl_slist_append(httpHeaders, "Content-Type: application/json");
    httpHeaders = curl_slist_append(httpHeaders, "Expect:");

    while (true) {
        // local init
        HTTPResponse response;
        FileWrapper postData(jsonQueue.pop(), false);

        CURL* curl = curl_easy_init();
        if (!curl) {
            LOG_ERR << "Failed to initialize libcurl";
            break;
        }

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
        curl_easy_setopt(curl, CURLOPT_READDATA, (void*)postData.get_file());

        // process response callback
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, parse_response);

        // process response argument
        LOG_INFO << "Sending POST request";
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&response);

        CURLcode code;
        if ((code = curl_easy_perform(curl)) != CURLE_OK) {
            LOG_ERR << "POST failed with error: " << curl_easy_strerror(code);
        } else {
            process_response(response);
        }

        curl_easy_cleanup(curl);
    }

    // global cleanup
    curl_slist_free_all(httpHeaders);
    curl_global_cleanup();
    LOG_ERR << "Terminating due to a failure";
    exit(EXIT_FAILURE);
}

curl_socket_t opensocket(void*, curlsocktype, struct curl_sockaddr*)
{
    static auto config = Config::get_config();
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
    localAddr.sin_port = htons(config["local_port"].get<int>());

    if (bind(sockfd, (struct sockaddr*)&localAddr, sizeof(localAddr))) {
        LOG_ERR << "Failed to bind local address";
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

size_t parse_response(void* data, size_t, size_t bufSize, void* userp)
{
    HTTPResponse* response = (HTTPResponse*)userp;
    return response->append_data(data, bufSize);
}

void process_response(const HTTPResponse& response)
{
    LOG_INFO << "Received POST response:\n" << response.get_data();
}
