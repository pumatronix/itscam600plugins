/*
 *  blocking_queue.cpp
 *
 *  Created on: Dec 02, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Thread Safe String Queue
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "helpers/blocking_queue.h"

//==============================================================================
// METHODS
//==============================================================================
void FilenameQueue::push(std::string value)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.push(value);

    // notify any thread waiting to read
    mEmptyCondition.notify_all();
}

std::string FilenameQueue::pop()
{
    std::unique_lock<std::mutex> lock(mMutex);

    // thread will wait until there is data to
    // be read then lock the mutex and proceed
    mEmptyCondition.wait(lock, [this] { return !mQueue.empty(); });

    std::string value = mQueue.front();
    mQueue.pop();
    return value;
}

bool FilenameQueue::empty() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.empty();
}

size_t FilenameQueue::size() const
{
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.size();
}
