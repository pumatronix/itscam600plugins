/*
 *  jtime_impl_linux.cpp
 *
 *  Created on: Nov 22, 2018
 *      Author: Felipe Gabriel G. Camargo - Pumatronix
 *
 *  JLib Time Related Functions - Linux Implementation
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "camera/jlib/jarch/hw_os.h"
#include "camera/jlib/jos/jtime.h"
#include "camera/jlib/jdebug/jdebug.h"

#include <sys/time.h>
#include <time.h>

//==============================================================================
// METHODS
//==============================================================================
namespace jtime {
uint64_t getTimeNs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec*1000000000UL) + ((uint64_t)ts.tv_nsec);
}

uint64_t getTimeUs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec*1000000UL) + ((uint64_t)ts.tv_nsec/1000UL);
}

uint64_t getTimeMs()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec*1000UL) + ((uint64_t)ts.tv_nsec/1000000UL);
}

// ISR Specific
uint64_t getTimeNsISR()
{
    return getTimeNs();
}

uint64_t getTimeUsISR()
{
    return getTimeUs();
}

uint64_t getTimeMsISR()
{
    return getTimeMs();
}
}/* end namespace jtime */

//==============================================================================
// LEGACY API
//==============================================================================
namespace utils {

double get_time_ms()
{
    return static_cast<double>(jtime::getTimeMs());
}

uint64_t get_time_us()
{
    return jtime::getTimeUs();
}

std::string get_formated_time()
{
    std::string buffer(32,0);

    struct tm timeinfo = {};
    struct timeval tv = {};

    /* get the current wall-clock time */
    gettimeofday(&tv, NULL);

    /* convert seconds since epoch to full time struct */
    localtime_r(&tv.tv_sec, &timeinfo);

    /* create formatted string and append microseconds */
    std::string tmp(26,0);
    strftime(&tmp[0], 26, "%Y:%m:%d %H:%M:%S", &timeinfo);
    snprintf(&buffer[0], buffer.size(), "%s.%06d", &tmp[0], (int)tv.tv_usec);

    return buffer;
}

uint64_t get_epoch_time()
{
    struct timeval tv = {};
    /* get the current wall-clock time */
    gettimeofday(&tv, NULL);
    /* convert seconds since epoch to full time struct */
    return (uint64_t) tv.tv_sec;
}

JTimestamp get_timestamp() 
{
    JTimestamp ts = {};
    struct timeval tv = {};
    struct tm tm = {};
    /* get the current wall-clock time */
    gettimeofday(&tv, NULL);
    /* convert seconds since epoch to full time struct */
    localtime_r(&tv.tv_sec, &tm);
    /* set return */
    ts.year     = tm.tm_year + 1900;    /* years since 1900 */
    ts.month    = tm.tm_mon + 1;        /* mounths since january */
    ts.day      = tm.tm_mday;
    ts.hour     = tm.tm_hour;
    ts.min      = tm.tm_min;
    ts.sec      = tm.tm_sec;
    ts.msec     = static_cast<uint16_t>(tv.tv_usec/1000);
    return ts;
}

JTimestamp get_utc_timestamp() 
{
    JTimestamp ts = {};
    struct timeval tv = {};
    struct tm tm = {};
    /* get the current wall-clock time */
    gettimeofday(&tv, NULL);
    /* convert seconds since epoch to full time struct */
    gmtime_r(&tv.tv_sec, &tm);
    /* set return */
    ts.year     = tm.tm_year + 1900;    /* years since 1900 */
    ts.month    = tm.tm_mon + 1;        /* mounths since january */
    ts.day      = tm.tm_mday;
    ts.hour     = tm.tm_hour;
    ts.min      = tm.tm_min;
    ts.sec      = tm.tm_sec;
    ts.msec     = static_cast<uint16_t>(tv.tv_usec/1000);
    return ts;
}

} /* end namespace utils */
