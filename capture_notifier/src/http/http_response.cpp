/*
 *  post_response.cpp
 *
 *  Created on: Dec 14, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  HTTP POST response
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/log.h"
#include "http/http_response.h"

//==============================================================================
// METHODS
//==============================================================================
HTTPResponse::HTTPResponse()
{
    mData = (char*)malloc(1);
    mSize = 0;
}

HTTPResponse::~HTTPResponse() { free(mData); }

size_t HTTPResponse::append_data(void* data, size_t bufSize)
{
    char* newData = (char*)realloc(mData, mSize + bufSize + 1);

    if (!newData) {
        LOG_ERR << "Out of memory!";
        exit(EXIT_FAILURE);
    }

    memcpy(&(newData[mSize]), data, bufSize);

    mData = newData;
    mSize += bufSize;
    mData[mSize] = 0;

    return bufSize;
}

const char* HTTPResponse::get_data() const { return mData; }
