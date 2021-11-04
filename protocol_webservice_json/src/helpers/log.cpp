/*
 *  log.cpp
 *
 *  Created on: Dec 03, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Minimal logger
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/log.h"

//==============================================================================
// STATIC MEMBERS
//==============================================================================
bool Log::mIsColorized = false;
std::mutex Log::mLogMutex;
std::ostream Log::mOstream(nullptr);
const std::string Log::mFormat = "%Y-%m-%d %a %H:%M:%S";

//==============================================================================
// METHODS
//==============================================================================
Log::Log(const char* file, int line, bool err, bool colorized)
{
    // lock before start writing
    mLogMutex.lock();

    mIsColorized = colorized;
    mOstream.rdbuf(err ? std::cerr.rdbuf() : std::cout.rdbuf());

    applyDateTime();
    mOstream << " ";
    applyFileLine(file, line);
    mOstream << " ";
    if (mIsColorized) {
        applyColor(err ? FG_RED : FG_GREEN);
    }
}

Log::~Log()
{
    if (mIsColorized) {
        removeColor();
    }

    mOstream << std::endl << std::flush;

    // unlock
    mLogMutex.unlock();
}

void Log::applyColor(int color) { mOstream << "\033[" << color << "m"; }

void Log::removeColor() { applyColor(FG_DEFAULT); }

void Log::applyDateTime()
{
    using namespace std::chrono;

    char mbstr[30];
    std::time_t now = system_clock::to_time_t(system_clock::now());
    std::strftime(mbstr, sizeof(mbstr), mFormat.c_str(), std::localtime(&now));

    mOstream << mbstr;
}

void Log::applyFileLine(const char* file, int line)
{
    mOstream << "[" << file << ":" << line << "]";
}
