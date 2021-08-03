/*
 *  comment_parser.h
 *
 *  Created on: Sep 29, 2017
 *      Author: Felipe Camargo - Pumatronix
 *
 *  ITSCAM JPEG Comments Parser
 *
 */
#ifndef __JVIDEO_ITSCAM_COMMENT_PARSER_H__
#define __JVIDEO_ITSCAM_COMMENT_PARSER_H__
//==============================================================================
//  INCLUDES
//==============================================================================
#include <map>
#include <string>
#include <vector>
#include "camera/jlib/jutils/jutils_strings.h"

//==============================================================================
//  CLASS
//==============================================================================
namespace utils {

class CommentParser {
private:

    std::string rawString;
    std::map<std::string, std::string> values;

public:

    CommentParser(const std::string& comments) {
        rawString = comments;
        parse();
    }

    CommentParser(std::vector<uint8_t>& jpeg) {
        rawString = jutils::getJpegCommentString(jpeg);
        parse();
    }

    CommentParser(const uint8_t* jpeg_ptr, size_t jpeg_size) {
        rawString = jutils::getJpegCommentString(jpeg_ptr, jpeg_size);
        parse();
    }

    ~CommentParser() {}

    bool has(const std::string& key) {
        return (values.find(key) != values.end());
    }

    std::string getString(const std::string& key, const std::string& dvalue="") {
        if(has(key)) {
            return values[key];
        } else {
            return dvalue;
        }
    }

    int getInt(const std::string& key, int dvalue=0) {
        if(has(key)) {
            return jutils::stringToInt(values[key]);
        } else {
            return dvalue;
        }
    }

    unsigned int getUInt(const std::string& key, unsigned int dvalue=0U) {
        if(has(key)) {
            return jutils::stringToUInt(values[key]);
        } else {
            return dvalue;
        }
    }

    float getFloat(const std::string& key, float dvalue=0.f) {
        if(has(key)) {
            return jutils::stringToFloat(values[key]);
        } else {
            return dvalue;
        }
    }

private:

    void parse() {
        std::vector<std::string> fields = jutils::split(rawString, ';');
        for(size_t i = 0; i < fields.size(); i++) {
            std::vector<std::string> tokens = jutils::split(fields[i], '=');
            if(tokens.size() == 2) {
                values[tokens[0]] = tokens[1];
            }
        }
    }

};

} /* end namespace utils*/

#endif /* __JVIDEO_ITSCAM_COMMENT_PARSER_H__ */
