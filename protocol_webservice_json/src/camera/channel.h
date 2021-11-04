/*
 *  channel.h
 *
 *  Created on: Jul 22, 2021
 *      Author: Alexandre Krzyzanovski - Pumatronix
 *
 *  Camera channel
 *
 */
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/blocking_queue.h"
#include <nlohmann/json.hpp>

//==============================================================================
// DEFINES
//==============================================================================
#define MAX_NUM_FRAMES 4
#define BUFFER_SIZE 2*1024*1024

//==============================================================================
// FUNCTIONS
//==============================================================================
void get_capture(CaptureVehicleQueue&, nlohmann::json&);
void image2json(CaptureVehicleQueue&, CaptureVehicleQueue&);

void updateImageCounter(int counter);
#endif /* __CHANNEL_H__ */
