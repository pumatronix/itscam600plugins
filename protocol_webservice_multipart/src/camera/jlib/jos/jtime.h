/*
 *  jtime.h - v2
 *
 *  Created on: Nov 29, 2018
 *      Author: Felipe Gabriel G. Camargo - Pumatronix
 *
 *  Time related utils
 *
 */
#ifndef __JTIME_H__
#define __JTIME_H__

//==============================================================================
// INCLUDE
//==============================================================================
#include "camera/jlib/jarch/hw_os.h"
#include <stdint.h>
#include <string>

//==============================================================================
// METHODS
//==============================================================================
namespace jtime {
/**
  * @brief
  * Retrieve system timer value since startup. The timer is guaranteed to be
  * monitonic but its resolution may vary between plaforms.
  * If running from inside an ISR, please use one of the ISR safe variants. In
  * non realtime systems, the ISR functions are alises to the common methods.
  *
  * @return
  * monotonic timer value in
  *     nanoseconds (getTimeNs)
  *     microseconds (getTimeUs)
  *     miliseconds (getTimeMs)
  */
uint64_t getTimeNs();
uint64_t getTimeUs();
uint64_t getTimeMs();
// ISR
uint64_t getTimeNsISR();
uint64_t getTimeUsISR();
uint64_t getTimeMsISR();
}

//==============================================================================
// @deprecated
// LEGACY API
//==============================================================================
namespace utils {

/* types */
struct JTimestamp {
    uint16_t year;
    uint16_t month;
    uint16_t day;
    uint16_t hour;
    uint16_t min;
    uint16_t sec;
    uint16_t msec;
    uint16_t pad; /* keep 32/64bits alignment */

    JTimestamp(
        uint16_t mYear=0, uint16_t mMonth=0, uint16_t mDay=0, uint16_t mHour=0,
        uint16_t mMin=0, uint16_t mSec=0, uint16_t mMsec=0) :
        year(mYear), month(mMonth), day(mDay), hour(mHour),
        min(mMin), sec(mSec), msec(mMsec), pad(0) {}
};

// Raw Time
double get_time_ms();
uint64_t get_time_us();

// Formated Time
uint64_t get_epoch_time();
std::string get_formated_time();
JTimestamp get_timestamp();
JTimestamp get_utc_timestamp();

}

#endif /* __JTIME_H__ */
