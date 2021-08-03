/*
 *  capture_notifier.cpp
 *
 *  Created on: Dec 01, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Capture Notifier Plugin Example
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "camera/channel.h"
#include "capture_notifier.h"
#include "config/config_parser.h"
#include "helpers/blocking_queue.h"
#include "helpers/log.h"
#include "http/handle_post.h"

#include <thread>

//==============================================================================
// Program Entry Point
//==============================================================================
int main(void)
{
    auto config = Config::get_config();

    LOG_INFO << "Configuration file:";
    for (auto& item : config.items()) {
        LOG_INFO << item.key() << " : " << item.value();
    }
    LOG_INFO << "-------------------";

    FilenameQueue imagesQueue;
    FilenameQueue jsonQueue;

    LOG_INFO << "Capture Notifier - Version " << PLUGIN_VERSION_MAJOR << "."
             << PLUGIN_VERSION_MINOR;

    std::thread gCapture(get_capture, std::ref(imagesQueue));
    std::thread jCapture(image2json, std::ref(imagesQueue), std::ref(jsonQueue));
    std::thread pCapture(post_capture, std::ref(jsonQueue));

    gCapture.join();
    jCapture.join();
    pCapture.join();

    return EXIT_SUCCESS;
}
