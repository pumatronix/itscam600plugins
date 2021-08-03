/*
 *  channel.cpp
 *
 *  Created on: Dec 11, 2020
 *      Author: Eduardo Almeida - Pumatronix
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
#include "helpers/log.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <thread>
#include <unordered_map>

//==============================================================================
// CONSTANTS
//==============================================================================
static uint8_t jsonBuf[BUFFER_SIZE];
static uint8_t imgBuf[MAX_NUM_FRAMES][BUFFER_SIZE];
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                        "abcdefghijklmnopqrstuvwxyz"
                                        "0123456789+/";

static std::unordered_map<std::string, std::string> metaDataMap;
//==============================================================================
// FUNCTIONS
//==============================================================================
static void getParsedMetadata(std::string);
static std::string base64Encode(const uint8_t*, int);

void get_capture(FilenameQueue& imagesQueue)
{
    LOG_INFO << "Start monitoring captures";
    Itscam300 camera("172.17.0.1", 0);
    int imageCounter = 0;
    int ret = 0;

    while (true) {
        std::string path = "/var/captures/capture_" + std::to_string(imageCounter);
        ret = camera.requisitaFotoIO((unsigned char*)imgBuf[0], 1, 60);

        if (ret <= 0) {
            // avoid high CPU usage on comunication error
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        std::ofstream imageFile(path);
        imageFile << std::string((char*)imgBuf[0], ret);

        imageCounter++;
        imagesQueue.push(path);
    }
}

void image2json(FilenameQueue& imagesQueue, FilenameQueue& jsonQueue)
{
    while (true) {
        FileWrapper image(imagesQueue.pop(), true);
        int charCount = 0, tagBegin = 0, currChar;

        // copy jpeg from image file
        while (true) {
            // get next char
            currChar = fgetc(image.get_file());

            // copy it
            jsonBuf[charCount] = currChar;

            // find metadata
            if (jsonBuf[charCount - 1] == 0xFF && jsonBuf[charCount] == 0xFE) {
                tagBegin = charCount + 3;
            }

            // EOF
            if (jsonBuf[charCount - 1] == 0xFF && jsonBuf[charCount] == 0xD9) {
                break;
            }

            charCount++;
        }

        // metadata not found
        if (!tagBegin)
            continue;

        std::string metaData((char*)&jsonBuf[tagBegin]);

        getParsedMetadata(metaData);

        nlohmann::json jsonData;
        jsonData["DATETIME"] = metaDataMap["Horario"];
        jsonData["PLATES"] = nlohmann::json::array();

        // array of plates
        std::string plates = metaDataMap["Placa"];
        std::string coords = metaDataMap["CoordPlaca"];
        std::string colors = metaDataMap["CorPlaca"];
        while (true) {
            nlohmann::json plateJson;
            plateJson["PLATE"] = plates.substr(0, plates.find("_"));
            plateJson["COORDS"] = coords.substr(0, coords.find("_"));
            plateJson["COLOR"] = colors.substr(0, colors.find("_"));

            // store on plates array
            jsonData["PLATES"].push_back(plateJson);

            // no more values to read
            if (plates.find("_") == std::string::npos)
                break;

            // keep going
            plates = plates.substr(plates.find("_") + 1);
            coords = coords.substr(coords.find("_") + 1);
            colors = colors.substr(colors.find("_") + 1);
        }

        // write image on base64
        jsonData["IMAGE"] = base64Encode(jsonBuf, charCount);

        std::string jsonPath = image.get_path() + "_json";
        std::ofstream postDataFile(jsonPath);
        postDataFile << jsonData << std::endl;

        jsonQueue.push(jsonPath);
    }
}

static void getParsedMetadata(std::string data)
{
    size_t posEquals, posLeft, posRight = -1;
    while (true) {
        std::string curKey, curValue;

        posEquals = data.find("=", posRight + 1);
        posLeft = posRight + 1;
        posRight = data.find(";", posRight + 1);

        if (posEquals == std::string::npos)
            break;

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
