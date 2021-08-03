/*
 *  channel.h
 *
 *  Created on: Dec 11, 2020
 *      Author: Eduardo Almeida - Pumatronix
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

//==============================================================================
// DEFINES
//==============================================================================
#define MAX_NUM_FRAMES 1
#define BUFFER_SIZE 2*1024*1024

//==============================================================================
// FUNCTIONS
//==============================================================================
void get_capture(FilenameQueue&);
void image2json(FilenameQueue&, FilenameQueue&);

#endif /* __CHANNEL_H__ */
