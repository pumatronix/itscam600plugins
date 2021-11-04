/*
 *  log.h
 *
 *  Created on: Dec 03, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Minimal logger
 *
 */
#ifndef __LOG_H__
#define __LOG_H__

//==============================================================================
// DEFINES
//==============================================================================
#define __FILENAME__                                                                               \
    (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define LOG_INFO Log(__FILENAME__, __LINE__, false, true)
#define LOG_ERR Log(__FILENAME__, __LINE__, true, true)

//==============================================================================
// INCLUDES
//==============================================================================
#include <iostream>
#include <mutex>

//==============================================================================
// CLASS
//==============================================================================

/**
 * @brief Log
 * This class should not be used by itself, use the macros defined above.
 *
 * USAGE: LOG_INFO << a << b << c;
 */
class Log {
private:
    // attributes
    static bool mIsColorized;
    static std::mutex mLogMutex;
    static std::ostream mOstream;
    static const std::string mFormat;

    enum Code {
        FG_DEFAULT = 0,
        FG_RED = 91,
        FG_GREEN = 92,
    };

    // methods
    void applyColor(int);
    void removeColor();
    void applyDateTime();
    void applyFileLine(const char*, int);

public:
    // methods
    Log(const char*, int, bool, bool);
    ~Log();

    template <class T> Log& operator<<(const T& message)
    {
        mOstream << message;
        return *this;
    }
};

#endif /* __LOG_H__ */
