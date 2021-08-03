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
FileWrapper::FileWrapper(std::string filePath, bool binary)
    : mPath(filePath)
{
    mFile = std::fopen(filePath.c_str(), binary ? "rb" : "r");
    if (!mFile) {
        LOG_ERR << "Error while reading file";
        exit(EXIT_FAILURE);
    }
}

FileWrapper::~FileWrapper()
{
    std::fclose(mFile);

    if (remove(mPath.c_str())) {
        LOG_ERR << "Failed to remove file " << mPath;
        exit(EXIT_FAILURE);
    }
}

FILE* FileWrapper::get_file() const { return mFile; }

std::string FileWrapper::get_path() const { return mPath; }
