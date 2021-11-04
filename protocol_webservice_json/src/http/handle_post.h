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
#include <nlohmann/json.hpp>

#include <curl/curl.h>

//==============================================================================
// FUNCTIONS
//==============================================================================
void post_capture(CaptureVehicleQueue& , nlohmann::json&);
curl_socket_t opensocket(void*, curlsocktype, struct curl_sockaddr*);

#endif /* __HANDLE_POST_H__ */
