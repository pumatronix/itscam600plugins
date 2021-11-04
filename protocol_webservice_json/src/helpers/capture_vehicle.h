/*
 *  capture_vehicle.h
 *
 *  Created on: Jul 29, 2021
 *      Author: Alexandre Krzyzanovski - Pumatronix
 *
 *  Capture Vehicle data
 *
 */
#ifndef __CAPTURE_VEHICLE_H__
#define __CAPTURE_VEHICLE_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include <cstdio>
#include <string>
#include <vector>

#include "helpers/file_handler.h"
//==============================================================================
// CLASS
//==============================================================================

/**
 * @brief Log
 * This class should handles data received in an HTTP POST
 */
class CaptureVehicle {
private:
    std::vector<std::string> plates;
    std::vector<FileWrapper*> jsons;
    std::vector<FileWrapper*> images;

public:
    CaptureVehicle() {

    }
    ~CaptureVehicle() {
        this->plates.clear();
        this->plates.shrink_to_fit();
        // force clear vector
        for(auto json : this->jsons) {
            delete json;
        }
        this->jsons.clear();
        this->jsons.shrink_to_fit();
        for(auto image : this->images) {
            delete image;
        }
        this->images.clear();
        this->images.shrink_to_fit();
    }

    void push_json(const std::string& plate, FileWrapper* json) {
        this->plates.push_back(plate);
        this->jsons.push_back(json);
    }

    std::vector<std::string> get_plates() {
        return this->plates;
    }
    std::vector<FileWrapper*> get_jsons() {
        return this->jsons;
    }
    std::vector<FileWrapper*> get_images() {
        return this->images;
    }

    void push_image(FileWrapper* image) {
        this->images.push_back(std::move(image));
    }

    bool empty() {
        return this->images.empty();
    }

    FileWrapper* front() {
        return this->images[0];
    }

};

#endif /* __CAPTURE_VEHICLE_H__ */
