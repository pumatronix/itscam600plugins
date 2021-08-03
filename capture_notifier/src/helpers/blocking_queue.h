/*
 *  blocking_queue.h
 *
 *  Created on: Dec 02, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Thread Safe String Queue
 *
 */
#ifndef __BLOCKING_QUEUE_H__
#define __BLOCKING_QUEUE_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include <condition_variable>
#include <queue>

//==============================================================================
// CLASS
//==============================================================================

/**
 * @brief FilenameQueue
 * Thread safe queue used to store file paths as strings
 */
class FilenameQueue {
public:
    bool empty() const;
    size_t size() const;
    std::string pop();
    void push(std::string);

private:
    mutable std::mutex mMutex;
    std::queue<std::string> mQueue;
    std::condition_variable mEmptyCondition;
};

#endif /* __BLOCKING_QUEUE_H__ */
