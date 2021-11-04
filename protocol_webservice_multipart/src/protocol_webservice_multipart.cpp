/*
 *  protocol_webservice_multipart.cpp
 *
 *  Created on: Nov 02, 2021
 *      Author: Alexandre Krzyzanovski - Pumatronix
 *
 *  Protocol to WebService Multipart Plugin Example
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "camera/channel.h"
#include "protocol_webservice_multipart.h"
#include "helpers/blocking_queue.h"
#include "helpers/log.h"
#include "http/handle_post.h"

#include "camera/jlib/jfs/filesystem.h"
#include "camera/jlib/jutils/jutils_strings.h"

#include <unordered_map>

#include <thread>
#include <iostream>
#include <string>
#include <regex>


void populateQueue(const std::string& root, CaptureVehicleQueue& jsonQueue) {
    std::map<int, CaptureVehicle*> pending;

    std::regex str_json_expr ("(\\/var\\/captures\\/)capture_(\\d+)_(\\d+)_([\\w\\d]+)_json");
    std::regex str_image_expr ("(\\/var\\/captures\\/)capture_(\\d+)_(\\d+)");
    std::vector<fs::FileInfo> ret = fs::listdir(root, true, fs::SORT_TIME_ASCENDING);
    
    std::smatch matcher;
    for(fs::FileInfo info: ret) {
        if (std::regex_match(info.abspath, matcher, str_json_expr)) {
            if (matcher.size() == 5) {
                std::ssub_match imageidfound = matcher[2];
                std::ssub_match platefound = matcher[4];
                int img = jutils::stringToInt(imageidfound.str());
                std::string plate = platefound.str();
                CaptureVehicle* capture = NULL;
                if(pending.find(img) == pending.end()) {
                    capture = new CaptureVehicle();
                    pending[img] = capture;
                } else {
                    capture = pending[img];
                }
                capture->push_json(plate, new FileWrapper(info.abspath, true, true));
                LOG_INFO << "Found " << info.abspath << " for plate " << plate << " image " << img << '\n';
            }
        } else if (std::regex_match(info.abspath, matcher, str_image_expr)) {
            if (matcher.size() == 4) {
                std::ssub_match imageidfound = matcher[2];
                int img = jutils::stringToInt(imageidfound.str());
                CaptureVehicle* capture = NULL;
                if(pending.find(img) == pending.end()) {
                    capture = new CaptureVehicle();
                    pending[img] = capture;
                } else {
                    capture = pending[img];
                }
                capture->push_image(new FileWrapper(info.abspath, true, true));
                LOG_INFO << "Found " << info.abspath << " image " << img << '\n';
            }
        }
    }

    int maxCounter = 0;
    for (auto it = pending.begin(); it != pending.end(); ++it) {
        int imgID = it->first;
        auto capture = it->second;
        if(imgID > maxCounter) {
            maxCounter = imgID;
        }
        if(capture->get_jsons().empty()) {
            // invalid capture
            delete capture;
            continue;
        }
        if(capture->get_images().empty()) {
            // invalid capture
            delete capture;
            continue;
        }
        jsonQueue.push(capture);
    }

    updateImageCounter(maxCounter);

    ret.clear();
    ret.shrink_to_fit();

    pending.clear();
}

//==============================================================================
// Program Entry Point
//==============================================================================
int main(int argc, char *argv[])
{
    nlohmann::json config = {
        {"camera_address", "172.17.0.1"},
        {"number_of_frames_day", 1},
        {"number_of_frames_night", 2},
        {"quality", 80},
        {"remote_addr", "192.168.254.2"},
        {"remote_port", 8081},
        {"remote_api_url", "/rest/multipart"}
   };
    if(argc>=2){
        config.merge_patch(nlohmann::json::parse(argv[1]));
    }

    LOG_INFO << "Configuration file:";
    for (auto& item : config.items()) {
        LOG_INFO << item.key() << " : " << item.value();
    }
    LOG_INFO << "-------------------";

    CaptureVehicleQueue imagesQueue;
    CaptureVehicleQueue jsonQueue;

    LOG_INFO << "Protocol to WebService Multipart Plugin - Version " << 
        PLUGIN_VERSION_MAJOR << "." << PLUGIN_VERSION_MINOR;

    populateQueue("/var/captures", jsonQueue);

    std::thread gCapture(get_capture, std::ref(imagesQueue), std::ref(config));
    std::thread jCapture(image2json, std::ref(imagesQueue), std::ref(jsonQueue));
    std::thread pCapture(post_capture, std::ref(jsonQueue), std::ref(config));

    gCapture.join();
    jCapture.join();
    pCapture.join();

    return EXIT_SUCCESS;
}
