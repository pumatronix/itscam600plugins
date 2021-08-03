/*
 *  handle_post.h
 *
 *  Created on: Dec 09, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Simple HTTP POST
 *
 */

#ifndef __HANDLE_POST_H__
#define __HANDLE_POST_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/blocking_queue.h"
#include "http/http_response.h"

#include <curl/curl.h>

//==============================================================================
// FUNCTIONS
//==============================================================================
void post_capture(FilenameQueue& captures_queue);
curl_socket_t opensocket(void*, curlsocktype, struct curl_sockaddr*);
size_t parse_response(void*, size_t, size_t, void*);
void process_response(const HTTPResponse&);

#endif /* __HANDLE_POST_H__ */
