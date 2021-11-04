/*
 *  file_handler.cpp
 *
 *  Created on: Dec 15, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  File handler (open, closes and removes files)
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/file_handler.h"
#include "helpers/log.h"

//==============================================================================
// METHODS
//==============================================================================
FileWrapper::FileWrapper(std::string filePath, bool binary, bool remove)
    : mPath(filePath),
      mRemove(remove)
{
    mFile = std::fopen(filePath.c_str(), binary ? "rb" : "r");
    if (!mFile) {
        LOG_ERR << "Error while reading file " << filePath;
    }
}

FileWrapper::~FileWrapper()
{
    std::fclose(mFile);

    if(mRemove) {
        if (remove(mPath.c_str())) {
            LOG_ERR << "Failed to remove file " << mPath;
        }
    }
}

FILE* FileWrapper::get_file() const { return mFile; }

std::string FileWrapper::get_path() const { return mPath; }

size_t FileWrapper::size() {
    size_t sz = 0;
    if(mFile) {
        fseek(mFile, 0L, SEEK_END);
        sz = ftell(mFile);
        fseek(mFile, 0L, SEEK_SET);
    }
    return sz;
}

int FileWrapper::get_content(uint8_t* buffer, size_t maxsize) {
    size_t sz = size();
    if(sz >= maxsize) {
        sz = maxsize;
    }
    fread(buffer, sz, 1, mFile);
    return sz;
}
