/*
 *  channel.cpp
 *
 *  Created on: Jul 22, 2021
 *      Author: Alexandre Krzyzanovski - Pumatronix
 *
 *  Camera channel
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "camera/channel.h"
#include "camera/itscam/itscam300.h"
#include "helpers/file_handler.h"
#include "helpers/capture_vehicle.h"
#include "helpers/log.h"

#include "camera/jlib/jutils/jutils_strings.h"
#include "camera/jlib/jfs/filesystem.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <unordered_map>

//==============================================================================
// CONSTANTS
//==============================================================================
static uint8_t jsonBuf[BUFFER_SIZE];
static uint8_t *imgBuf[MAX_NUM_FRAMES];
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static std::unordered_map<std::string, std::string> metaDataMap;

static int imageCounter = 0;
//==============================================================================
// FUNCTIONS
//==============================================================================
static void getParsedMetadata(std::string data) {
    size_t posEquals, posLeft, posRight = -1;
    while (true) {
        std::string curKey, curValue;

        posEquals = data.find("=", posRight + 1);
        posLeft = posRight + 1;
        posRight = data.find(";", posRight + 1);

        if (posEquals == std::string::npos) {
            break;
        }

        curKey = data.substr(posLeft, posEquals - posLeft);
        curValue = data.substr(posEquals + 1, posRight - posEquals - 1);

        metaDataMap[curKey] = curValue;
    }
}

/*
 *    base64.cpp and base64.h
 *
 *    Copyright (C) 2004-2008 René Nyffenegger
 *    René Nyffenegger rene.nyffenegger@adp-gmbh.ch
 */
static std::string base64Encode(const uint8_t* data, int dataLen)
{
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t arr3[3];
    uint8_t arr4[4];

    while (dataLen--) {
        arr3[i++] = *(data++);

        if (i == 3) {
            arr4[0] = (arr3[0] & 0xfc) >> 2;
            arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
            arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
            arr4[3] = arr3[2] & 0x3f;

            for (i = 0; i < 4; i++)
                ret += base64_chars[arr4[i]];

            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++)
            arr3[j] = '\0';

        arr4[0] = (arr3[0] & 0xfc) >> 2;
        arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
        arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
        arr4[3] = arr3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[arr4[j]];

        while (i++ < 3)
            ret += '=';
    }

    return ret;
}

void updateImageCounter(int counter) {
    imageCounter = counter;
}

void get_capture(CaptureVehicleQueue& imagesQueue, nlohmann::json& config)
{
    // allocate memory
    for(int i = 0; i < MAX_NUM_FRAMES; i++) {
        imgBuf[i] = new uint8_t[BUFFER_SIZE];
    }

    int ret = 0;
    int lastDayNight = -1;

    auto itscam600 = config["camera_address"].get<std::string>();
    auto numberOfFrames = config["number_of_frames_day"].get<int>();
    auto quality = config["quality"].get<int>();

    LOG_INFO << "Connecting on ITSCAM 600[" << itscam600 << "]";
    
    while (true) {
        Itscam300 camera(itscam600.c_str(), 0, 50000);
        camera.setaTimeoutIO(5);

        if(camera.leStatus() != 0) {
            LOG_INFO << "Failure to connect " << itscam600;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            continue;
        }

        char mac_str[30];
        ret = 0;
        while (!ret) {
            ret = camera.leMac(mac_str);
            if(!ret) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        LOG_INFO << "Connected on " << itscam600 << " [" << mac_str << "]";

        while (true) {
            int size[MAX_NUM_FRAMES] = { 0, 0, 0, 0 };

            int currentDayNight = camera.leSituacaoDayNight();
            if(lastDayNight < 0) {
                lastDayNight = currentDayNight;
                LOG_INFO << "" << itscam600 << " [ Day/Night: " << currentDayNight << "]";
            } else if(lastDayNight != currentDayNight) {
                lastDayNight = currentDayNight;
                LOG_INFO << "" << itscam600 << " changed [ Day/Night: " << currentDayNight << "]";
            }

            if(currentDayNight != 1) {// [Day == 1] {
                numberOfFrames = config["number_of_frames_night"].get<int>();
            } else {
                numberOfFrames = config["number_of_frames_day"].get<int>();
            }
            
            ret = camera.requisitaMultiplasFotosIO((unsigned char**)imgBuf, numberOfFrames, size, 1, quality);

            if (ret > 0) {
                auto capture = new CaptureVehicle();
                for(int i = 0; i < numberOfFrames; i++) {
                    if(size[i] > 0) {
                        std::string path = "/var/captures/capture_" + std::to_string(imageCounter) + "_" + std::to_string(i);
                        LOG_INFO << "Save Frame => " << ret << " [" << path << " ("<< size[i] << ")]";

                        std::ofstream imageFile(path);
                        imageFile << std::string((char*)imgBuf[i], size[i]);
                        
                        capture->push_image(new FileWrapper(path, true, true));
                    }
                }
                imagesQueue.push(capture);
                imageCounter++;
            } else if(ret == -TIMEOUT_EXCEEDED) {
                // TIMEOUT... it's ok!
            } else {
                LOG_INFO << "requisitaMultiplasFotosIO ERROR => (" << ret << ")";
                // avoid high CPU usage on comunication error
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                break;
            }

        }
    }

    // Deallocate Memory
    for(int i = 0; i < MAX_NUM_FRAMES; i++) {
        delete[] imgBuf[i];
    }
    LOG_ERR << "Terminating due to a failure";
    exit(EXIT_FAILURE);
}

void image2json(CaptureVehicleQueue& imagesQueue, CaptureVehicleQueue& jsonQueue) 
{
    LOG_INFO << "Start Image 2 JSON process";
    while (true) {
        CaptureVehicle* capture = imagesQueue.pop();
        if(!capture->empty()) {
            FileWrapper* image = capture->front();
            int charCount = 0, tagBegin = 0, currChar;

            // copy jpeg from image file
            while (true) {
                // get next char
                currChar = fgetc(image->get_file());
                // copy it
                jsonBuf[charCount] = currChar;
                if(charCount  > 0) {
                    // find metadata
                    if (jsonBuf[charCount - 1] == 0xFF && jsonBuf[charCount] == 0xFE) {
                        tagBegin = charCount + 3;
                    }
                    // EOF
                    if (jsonBuf[charCount - 1] == 0xFF && jsonBuf[charCount] == 0xD9) {
                        break;
                    }
                }
                charCount++;
            }

            // metadata not found
            if (!tagBegin) {
                continue;
            }

            std::string metaData((char*)&jsonBuf[tagBegin]);

            getParsedMetadata(metaData);

            nlohmann::json jsonData;
            // TempoCaptura in microseconds
            auto tempoCapturaTag = metaDataMap["TempoCaptura"];
            auto tempoCaptura = jutils::stringToUInt64(tempoCapturaTag, 0);
            if(tempoCaptura == 0L) {
                tempoCaptura = utils::get_time_us();
            }
            // convert to milliseconds
            tempoCaptura = tempoCaptura / 1000UL;
            auto milliseconds = (int)(tempoCaptura % 1000UL);

            std::string datetime = metaDataMap["Horario"];
            int day;
            int month;
            int year;
            int hour;
            int minute;
            int second;

            // 26/07/21 17:53:50
            sscanf(datetime.c_str(), "%02d/%02d/%02d %02d:%02d:%02d", 
            &day, &month, &year, &hour, &minute, &second);
            jsonData["captureTime"] = jutils::toStringFormat("%04d-%02d-%02dT%02d:%02d:%02d.%03d-03:00", year+2000, month, day, hour, minute, second, milliseconds);

            // array of plates
            std::string platesRaw = metaDataMap["Placa"];
            std::string platesCoordRaw = metaDataMap["CoordPlaca"];
            std::replace( platesCoordRaw.begin(), platesCoordRaw.end(), 'x', ','); // replace all 'x' to ','
            std::string plates;
            if (metaDataMap.find("PlateMV") != metaDataMap.end()) {
                // if we have the field PlateMV.. we must use it even if it's empty...
                plates = metaDataMap["PlateMV"];
            } else {
                plates = metaDataMap["Placa"];
            }
            if(plates.empty()) {
                plates = "0000000";
            }

            auto imageList = nlohmann::json::array();
            for ( const auto& imagetemp : capture->get_images() ) {
                if(!fs::exists(imagetemp->get_path())) {
                    continue;
                }
                imagetemp->get_content(jsonBuf, BUFFER_SIZE);
                imageList.push_back(base64Encode(jsonBuf, charCount));
            }
            jsonData["images"] = imageList;

            std::vector<std::string> platesvm = jutils::split(plates, '_');

            std::vector<std::string> platesRawList = jutils::split(platesRaw, '_');
            std::vector<std::string> platesCoordRawList = jutils::split(platesCoordRaw, '_');

            if(!platesvm.empty()) {
                for (std::vector<std::string>::iterator it = platesvm.begin() ; it != platesvm.end(); ++it) {
                    std::string plate = *it;
                    jsonData["plate"] = plate;

                    for(size_t plateid = 0; plateid < platesRawList.size(); plateid++) {
                        if(platesRawList[plateid] == plate) {
                            jsonData["platePosition"] = platesCoordRawList[plateid];
                        }
                    }

                    std::string jsonPath = image->get_path() + "_" + plate +"_json";
                    std::ofstream postDataFile(jsonPath);
                    postDataFile << jsonData << std::endl;

                    capture->push_json(plate, new FileWrapper(jsonPath, true, true));
                }
            } else {
                std::string jsonPath = image->get_path() + "_json";
                std::ofstream postDataFile(jsonPath);
                postDataFile << jsonData << std::endl;

                capture->push_json("0000000", new FileWrapper(jsonPath, true, true));
            }
        }
        jsonQueue.push(capture);
    }
    LOG_ERR << "Terminating due to a failure";
    exit(EXIT_FAILURE);
}

