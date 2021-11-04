/*
 *  jutils_strings.h
 *
 *  Created on: May 27, 2016
 *      Author: Felipe Gabriel G. Camargo
 *
 *  String manipulation helpers
 *
 */
#ifndef __JUTILS_STRINGS__
#define __JUTILS_STRINGS__

//==============================================================================
//  INCLUDES
//==============================================================================
#include "camera/jlib/jarch/hw_os.h"
#include "camera/jlib/jos/jtime.h"
#include "camera/jlib/jdebug/jdebug.h"

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <string.h>
#include <stdint.h>
#include <cctype>
#include <cstdarg>

#ifdef PRIu64
#undef PRIu64
#endif

#ifdef WIN32
#define PRIu64 "I64u"
#else
#define PRIu64 "llu"
#endif

#ifdef PRId64
#undef PRId64
#endif

#ifdef WIN32
#define PRId64 "I64d"
#else
#define PRId64 "lld"
#endif


#ifdef WIN32
typedef unsigned int priu64_t;
#else
typedef unsigned long long priu64_t;
#endif

#ifdef DSP64x
#define TOUPPER std::toupper
#define TOLOWER std::tolower
#else
#define TOUPPER ::toupper
#define TOLOWER ::tolower
#endif

//==============================================================================
// INLINE FUNCTIONS
//==============================================================================
namespace jutils {

inline std::string join(const std::vector<std::string>& words, const std::string& delim) {
    std::string phrase;
    for (size_t i=0;i<words.size();i++) {
        phrase += words[i];
        if(i < words.size()-1) {
            phrase += delim;
        }
    }
    return phrase;
}

inline std::string replace(std::string text, const std::string& oldstr, const std::string& newstr) {
    size_t start_pos = text.find(oldstr);
    while(start_pos != std::string::npos) {
        text.replace(start_pos, oldstr.size(), newstr);
        start_pos = text.find(oldstr);
    }
    return text;
}

inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        if(!item.empty()) {
            elems.push_back(item);
        }
    }
    return elems;
}

inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

inline std::vector<std::string> splitOnFirst(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::size_t idx = s.find(delim);
    if(idx != std::string::npos) {
        elems.push_back(std::string(s.begin(), s.begin()+idx));
        elems.push_back(std::string(s.begin()+idx+1,s.end()));
    }
    return elems;
}

inline std::vector<std::string> splitOnLast(const std::string &s, char delim) {
    std::vector<std::string> elems;
    std::size_t idx = s.find_last_of(delim);
    if(idx != std::string::npos) {
        elems.push_back(std::string(s.begin(), s.begin()+idx));
        elems.push_back(std::string(s.begin()+idx+1,s.end()));
    }
    if(elems.size()==0){
        elems.push_back("");
    }
    return elems;
}

inline std::string shortFilename(const std::string &s) {
    std::vector<std::string> itens = split(s, '/');
    if(itens.empty()) {
        return s;
    } else {
        return itens.back();
    }
}

inline std::string& trimChar(std::string& str, const char c) {
    str.erase(std::remove(str.begin(), str.end(), c), str.end());
    return str;
}

inline std::string& trimChar(std::string& str, const std::vector<char>& clist) {
    for(size_t i = 0; i < clist.size(); i++) {
        str.erase(std::remove(str.begin(), str.end(), clist[i]), str.end());
    }
    return str;
}

/* convert string to uppercase */
inline std::string uppercase(std::string str) {
    for(size_t i = 0; i < str.size(); i++) {
        str[i] = TOUPPER(str[i]);
    }
    return str;
}

/* convert string to lowercase */
inline std::string lowercase(std::string str) {
    for(size_t i = 0; i < str.size(); i++) {
        str[i] = TOLOWER(str[i]);
    }
    return str;
}

inline bool contains(std::string& str1, std::string str2){
    if (str1.find(str2) != std::string::npos) {
        return true;
    } else {
        return false;
    }
}

inline bool endsWith(const std::string& str1, const std::string& str2){
    bool rc = true;
    if(str1.size() >= str2.size()) {
        for(size_t i = 0; i < str2.size(); i++) {
            if(str1[str1.size()-str2.size()+i] != str2[i]) {
                rc = false;
                break;
            }
        }
    } else {
        rc = false;
    }
    return rc;
}

inline std::vector<std::string> removeIfContains(const std::vector<std::string> &lines, std::string content) {
    std::vector<std::string> filteredLines;
    for(size_t i = 0; i < lines.size(); i++) {
        if(lines[i].find(content) == std::string::npos) {
            filteredLines.push_back(lines[i]);
        }
    }
    return filteredLines;
}

inline int stringToInt(const std::string& str, int dValue=0) {
    int ret = dValue;
    sscanf(str.c_str(), "%d", &ret);
    return ret;
}

inline int64_t stringToInt64(const std::string& str, int64_t dValue=0) {
    int64_t ret = dValue;
    std::stringstream ss(str);
    ss >> ret;
    if(ss.fail()) {
        ret = dValue;
    }
    return ret;
}

inline unsigned int stringToUInt(const std::string& str, unsigned int dValue=0) {
    unsigned int ret = dValue;
    sscanf(str.c_str(), "%u", &ret);
    return ret;
}

inline uint64_t stringToUInt64(const std::string& str, uint64_t dValue=0) {
    uint64_t ret = dValue;
    std::stringstream ss(str);
    ss >> ret;
    if(ss.fail()) {
        ret = dValue;
    }
    return ret;
}

inline float stringToFloat(const std::string& str, float dValue=0.0f) {
    float ret = dValue;
    sscanf(str.c_str(), "%f", &ret);
    return ret;
}

inline std::string intToString(int v) {
    char tmp[64] = {0};
    snprintf(tmp, sizeof(tmp), "%d", v);
    return std::string(tmp, strlen(tmp));
}

inline std::string uintToString(unsigned int v) {
    char tmp[64] = {0};
    snprintf(tmp, sizeof(tmp), "%u", v);
    return std::string(tmp, strlen(tmp));
}

inline std::string uint64ToString(uint64_t num, bool hex=false, size_t pad=8) {
    uint64_t sum = num;
    uint64_t base = hex ? 16 : 10;
    int digit;
    std::string str;
    do {
        digit = sum % base;
        if (digit < 0xA) {
            str.push_back('0' + digit);
        } else {
            str.push_back('A' + digit - 0xA);
        }
        sum /= base;
    } while (sum);
    if(str.size() < pad) {
        for(size_t i = 0; i < (str.size() - pad); i++) {
            str.push_back('0');
        }
    }
    std::reverse(str.begin(), str.end());
    return str;
}

inline std::string floatToString(float v) {
    char tmp[64] = {0};
    snprintf(tmp, sizeof(tmp), "%.2f", v);
    return std::string(tmp, strlen(tmp));
}

inline std::string doubleToString(double v) {
    char tmp[64] = {0};
    snprintf(tmp, sizeof(tmp), "%lf", v);
    return std::string(tmp, strlen(tmp));
}

template<typename T>
std::string toStringFmt(const T value, const char* fmt) {
    std::string str(64,'\0');
    size_t n = snprintf(&str[0],str.size(),fmt,value);
    if(n < str.size()) {
        str.resize(n);
    }
    return str;
}

inline std::string toStringFormat(const char* fmt, ...) {
    char buffer[512];
    int size = sizeof(buffer);
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    va_end(vl);

    if((nsize > 0) && (nsize < size)) {
        return std::string(buffer);
    } else {
        DEBUG_PERROR("Failed to format");
        return "";
    }
}

#ifndef DSP64x
inline std::string format(const std::string& fmt, ...) {
    char buffer[512];
    int size = sizeof(buffer);
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt.c_str(), vl);
    va_end(vl);
    if((nsize > 0) && (nsize < size)) {
        return std::string(buffer);
    } else {
        DEBUG_PERROR("Failed to format");
        return "";
    }
}
#endif

inline uint16_t getBigEndianUint16(const uint8_t* src, size_t start_pos) {
   uint16_t p0 = src[start_pos] & 0xFF;
   uint16_t p1 = src[start_pos+1] & 0xFF;
   return p1 | (p0 << 8);
}

/* extract the comments section of a JPEG file */
inline std::string getJpegCommentString(const uint8_t* jpeg_ptr, size_t jpeg_size) {
    /*
    JPEG Comment Structure
    [.....]
    [Comment begin] 0xFF, 0xFE
    [Comment size] 2 Bytes
    [Comment body] N bytes
    [.....]
    [End-of-image] 0xFF, 0xD9
    */
    std::string comment;
    if(jpeg_size > 2) {
        /* check if the jpeg is correctly ended */
        if(jpeg_ptr[jpeg_size-1] == 0xD9) {
            /* iterate backwards to improve parsing speed */
            for (size_t i = jpeg_size-1; i > 0; i--) {
                /* search for [Comment begin] */
                if((jpeg_ptr[i-1] == 0xFF) && (jpeg_ptr[i] == 0xFE)) {
                    /* comment body begins 3 bytes after [Comment begin] end */
                    size_t pos = i+3;
                    uint16_t comment_size = getBigEndianUint16(jpeg_ptr, i+1);
                    /* copy comment to the return string */
                    if (pos < jpeg_size) {
                        comment = std::string(jpeg_ptr + pos, jpeg_ptr + pos + comment_size - 2U);
                    }
                    break;
                }
            }
        }
    }
    return comment;
}

inline std::string getJpegCommentString(const std::vector<uint8_t>& jpeg) {
    return getJpegCommentString(&jpeg[0], jpeg.size());
}

/* format JTimestamp as yyyy-MM-dd​ ​ HH:mm:ss */
inline std::string jtimestampToDatetime(const utils::JTimestamp& timestamp) {
    return
    toStringFmt(timestamp.year,"%04d-") +
    toStringFmt(timestamp.month,"%02d-") +
    toStringFmt(timestamp.day,"%02d ") +
    toStringFmt(timestamp.hour,"%02d:") +
    toStringFmt(timestamp.min,"%02d:") +
    toStringFmt(timestamp.sec,"%02d");
}

} /* end namespace utils */

#endif /* __JUTILS_STRINGS__ */
