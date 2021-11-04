/*
 *  file_handler.h
 *
 *  Created on: Dec 15, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  File handler (open, closes and removes files)
 *
 */
#ifndef __FILE_HANDLER_H__
#define __FILE_HANDLER_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include <cstdio>
#include <string>

//==============================================================================
// CLASS
//==============================================================================

/**
 * @brief Log
 * This class should handles data received in an HTTP POST
 */
class FileWrapper {
private:
    FILE* mFile;
    std::string mPath;
    bool mRemove;

public:
    FileWrapper(std::string, bool, bool);
    ~FileWrapper();

    FILE* get_file() const;
    std::string get_path() const;

    size_t size();
    int get_content(uint8_t* buffer, size_t maxsize);
};

#endif /* __FILE_HANDLER_H__ */
