/*
 *  hw_os.h
 *
 *  Created on: Aug 21, 2015
 *      Author: Felipe Gabriel G. Camargo - Pumatronix
 *
 *  Basic Hardware Dependet SO Abstraction Layer
 *
 */
#ifndef __HW_OS_H__
#define __HW_OS_H__

//==============================================================================
// COMMON DEFINITIONS
//==============================================================================
typedef unsigned long ul_t;
typedef unsigned long long ull_t;

//==============================================================================
// C++ Version
//==============================================================================
#define jarch_CPP98_OR_GREATER  ( __cplusplus >= 199711L )
#define jarch_CPP11_OR_GREATER  ( __cplusplus >= 201103L )
#define jarch_CPP14_OR_GREATER  ( __cplusplus >= 201402L )
#define jarch_CPP17_OR_GREATER  ( __cplusplus >= 201703L )
#define jarch_CPP20_OR_GREATER  ( __cplusplus >= 202000L )

//==============================================================================
// DSP64x - TI
//==============================================================================
#ifdef DSP64x
#   include "impl/hw_os_dsp64x.h"
#endif
//==============================================================================
// LINUX/ANDROID
//==============================================================================
#if (defined(__linux__) || defined(ANDROID)) && defined(__GNUC__)
#   include "impl/hw_os_linux.h"
#endif
//==============================================================================
// WINDOWS (MINGW)
//==============================================================================
#if defined(WIN32) || defined(_WIN32) || defined(__MINGW32__)
#   include "impl/hw_os_windows.h"
#endif
//==============================================================================
// FreeRTOS (Xilinx)
//==============================================================================
#if defined(OS_FREERTOS)
#   include "impl/hw_os_freertos.h"
#endif

#endif /* __HW_OS_H__ */
