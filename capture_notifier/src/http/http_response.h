/*
 *  post_response.h
 *
 *  Created on: Dec 14, 2020
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  HTTP POST response
 *
 */

#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

//==============================================================================
// CLASS
//==============================================================================

/**
 * @brief Log
 * This class should handles data received in an HTTP POST
 */
class HTTPResponse {
private:
    char* mData;
    size_t mSize;

public:
    HTTPResponse();
    ~HTTPResponse();

    size_t append_data(void*, size_t);
    const char* get_data() const;
};

#endif /* __HTTP_RESPONSE_H__ */
