/*
 *  filesystem.h
 *
 *  Created on: Apr 12, 2020
 *      Author: Felipe Camargo - Pumatronix
 *
 *  libcamerahal - Filesystem manipulation methods
 *
 */
#ifndef __CAMERA_HAL_FILE_FILESYSTEM_H__
#define __CAMERA_HAL_FILE_FILESYSTEM_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include <string>
#include <vector>
#include <functional>
#include <time.h>

#include "camera/jlib/jos/jtime.h"

//==============================================================================
// NAMESPACE
//==============================================================================
namespace fs {

// Types
struct FileInfo {
    std::string name;
    std::string basedir;
    std::string relpath;
    std::string abspath;
    uint32_t size;
    time_t rawtime;
    utils::JTimestamp timestamp; // UTC (last modified)
};

enum Sort {
    SORT_NONE = 0,
    SORT_TIME_ASCENDING,
    SORT_TIME_DESCENDING,
    SORT_NAME_ASCENDING,
    SORT_NAME_DESCENDING
};

// Functions
bool exists(const std::string& path);
bool isdir(const std::string& path);
bool mkdir(const std::string& path);
bool mkpath(const std::string& path);
bool rename(const std::string& oldpath, const std::string& newpath);
FileInfo fileinfo(const std::string& path);
std::string getcwd();
std::string realpath(const std::string& path);
std::string basename(const std::string& path);
std::string dirname(const std::string& path);

void rmtree(const std::string& path, bool emptyOnly = false);
bool rmfile(const FileInfo& info);
bool rmfile(const std::string& path);
std::vector<FileInfo> listdir(const std::string& path, bool recursive = false, Sort sort = SORT_NONE);
std::vector<FileInfo> listdir(const std::string& path, const std::string& sufix, bool recursive = false, Sort sort = SORT_NONE);
bool filewalker(const std::string& path, bool recursive, std::function<void(const FileInfo&)> fn);
std::string joinPath(const std::string& p1, const std::string& p2);
size_t availableSpace(const std::string& path);

// Time utilities
utils::JTimestamp fileAge(const FileInfo& info);
bool isFileOlderThan(const FileInfo& info, const utils::JTimestamp& ts);
bool isFileOlderThan(const FileInfo& info, int y, int m, int d, int hh, int mm, int ss);
std::string toString(const utils::JTimestamp& ts);

namespace time_literals {

const utils::JTimestamp operator "" _YEAR(unsigned long long int y);
const utils::JTimestamp operator "" _MONTH(unsigned long long int m);
const utils::JTimestamp operator "" _DAY(unsigned long long int d);
const utils::JTimestamp operator "" _HOUR(unsigned long long int h);
const utils::JTimestamp operator "" _MIN(unsigned long long int mm);
const utils::JTimestamp operator "" _SEC(unsigned long long int ss);

} /* end namespace time_literals */

} /* end namespace fs */

#endif /* __CAMERA_HAL_FILE_FILESYSTEM_H__ */
