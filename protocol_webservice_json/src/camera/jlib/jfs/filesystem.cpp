/*
 *  filesystem.cpp
 *
 *  Created on: Apr 12, 2020
 *      Author: Felipe Camargo - Pumatronix
 *
 *  libcamerahal - Filesystem manipulation methods
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "camera/jlib/jfs/filesystem.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <algorithm>

#include "camera/jlib/jutils/jutils_strings.h"
#include "camera/jlib/jdebug/jdebug.h"

//==============================================================================
// NAMESPACE
//==============================================================================
namespace fs {

// Helpers
utils::JTimestamp toJTimestamp(const struct tm& tm, long nsec)
{
    utils::JTimestamp timestamp;
    /* set return */
    timestamp.year     = tm.tm_year + 1900;    /* years since 1900 */
    timestamp.month    = tm.tm_mon + 1;        /* months since january */
    timestamp.day      = tm.tm_mday;
    timestamp.hour     = tm.tm_hour;
    timestamp.min      = tm.tm_min;
    timestamp.sec      = tm.tm_sec;
    timestamp.msec     = static_cast<uint16_t>(nsec/1000000);
    return timestamp;
}

utils::JTimestamp toJTimestamp(const struct timespec& ts) 
{
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    gmtime_r(&ts.tv_sec, &tm);
    return toJTimestamp(tm,ts.tv_nsec);
}

std::string toString(const utils::JTimestamp& ts)
{
    std::string ret(32,0);
    snprintf(&ret[0], ret.size(), "%04uy%02um%02d %02uh%02um%02u.%03us", 
        (uint32_t)ts.year,
        (uint32_t)ts.month,
        (uint32_t)ts.day,
        (uint32_t)ts.hour,
        (uint32_t)ts.min,
        (uint32_t)ts.sec,
        (uint32_t)ts.msec);
    return ret;
}

// Functions
std::string realpath(const std::string& path) {
    if(path.empty()) return "";
    std::string pp = path;
    char* p = ::realpath(pp.c_str(), NULL);
    if(p) {
        std::string ret(p,strlen(p));
        ::free(p);
        return ret;
    }
    return "";
}

std::string basename(const std::string& path) {
    if(path.empty()) return "";
    std::string pp = path; pp.push_back('\0');
    char* p = &pp[0];
    char* b = ::basename(p);
    return std::string(b,strlen(b));
}

std::string dirname(const std::string& path) {
    if(path.empty()) return "";
    std::string pp = path; pp.push_back('\0');
    char* p = &pp[0];
    char* b = ::dirname(p);
    return std::string(b,strlen(b));
}

std::string joinPath(const std::string& p1, const std::string& p2) {
    if(p1.back() == '/') {
        return p1 + p2;
    } else {
        return p1 + "/" + p2;
    }
}

// Return filesystem available size in bytes given a path
size_t availableSpace(const std::string& path) {
    struct statvfs stat;
    if (::statvfs(path.c_str(), &stat) != 0) {
        return 0;
    }
    return stat.f_bsize * stat.f_bavail;
}

utils::JTimestamp fileAge(const FileInfo& info)
{
    utils::JTimestamp age;
    // wall time
    struct timeval now;
    memset(&now, 0, sizeof(struct timeval));

    gettimeofday(&now, NULL);
    // raw difference in secods
    double diff = difftime(now.tv_sec,info.rawtime);
    if(diff < 0) {
        DEBUG_PERROR("Inconsistent file timestamp, diff < 0");
        return age;
    }

    // Structure difference
    time_t diff_sec = static_cast<time_t>(diff);
    struct tm diff_tm;
    memset(&diff_tm, 0, sizeof(struct tm));
    gmtime_r(&diff_sec, &diff_tm);
    // Subtract baseline 1970:01:01 => 70:00:01
    age.year     = diff_tm.tm_year - 70; /* years since 1900 */
    age.month    = diff_tm.tm_mon;       /* months since january */
    age.day      = diff_tm.tm_mday - 1;
    age.hour     = diff_tm.tm_hour;
    age.min      = diff_tm.tm_min;
    age.sec      = diff_tm.tm_sec;

    return age;
}

bool isFileOlderThan(const FileInfo& info, const utils::JTimestamp& ts) {
    return isFileOlderThan(info, ts.year, ts.month, ts.day, ts.hour, ts.min, ts.sec);
}

bool isFileOlderThan(const FileInfo& info, int y, int m, int d, int hh, int mm, int ss) {
    auto age = fileAge(info);

    uint64_t age_sec =  age.year * (365U*24U*60U*60U) +
                        age.month * (30*24U*60U*60U) +
                        age.day * (24U*60U*60U) +
                        age.hour * (60U*60U) +
                        age.min * (60U) +
                        age.sec;
    uint64_t max_sec =  y * (365U*24U*60U*60U) +
                        m * (30*24U*60U*60U) +
                        d * (24U*60U*60U) +
                        hh * (60U*60U) +
                        mm * (60U) +
                        ss;
    return age_sec > max_sec;
}

bool pathExists(const std::string& path) {
    bool rc = false;
#if defined(WIN32)
    if(GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        rc = true;
    }
#elif defined(__linux__)
    if(access(path.c_str(), F_OK) == 0) {
        rc = true;
    }
#else
    DEBUG_PERROR("NOT IMPLEMENTED");
#endif
    return rc;
}

bool exists(const std::string& path) {
    return pathExists(path);
}

bool isdir(const std::string& path) {
    struct stat s;
    stat(path.c_str(), &s);
    return S_ISDIR(s.st_mode) ? true : false;
}

bool mkdir(const std::string& path) {
    return (::mkdir(path.c_str(), 0666) != -1);
}

bool mkpath(const std::string& path) {
    std::string fullpath;
    // system mkdir required absolute path
    if(path.find("./") == 0) {
        fullpath = joinPath(getcwd(),path);
    } else {
        fullpath = path;
    }
    std::string cmd = "mkdir -p " + fullpath;
    return system(cmd.c_str()) == 0;
}

bool rename(const std::string& oldpath, const std::string& newpath) {
    return (::rename(oldpath.c_str(), newpath.c_str()) != -1);
}

FileInfo fileinfo(const std::string& path) {
    FileInfo info;
    info.name = "";
    info.basedir = "";
    info.relpath = "";
    info.abspath = "";
    info.size = 0;
    info.rawtime = 0;
    info.timestamp = utils::JTimestamp();
    struct stat stat_entry;
    // Check
    if(exists(path)) return info;
    stat(path.c_str(), &stat_entry);
    if(!S_ISREG(stat_entry.st_mode)) return info;

    // Crete FileInfo
    info.name = basename(path.c_str());
    info.basedir = dirname(path.c_str());
    info.relpath = path;
    info.abspath = realpath(path);
    info.size = static_cast<uint32_t>(stat_entry.st_size);
    info.rawtime = stat_entry.st_mtime;
    info.timestamp = toJTimestamp(stat_entry.st_mtim);

    return info;
}

std::string getcwd() {
    std::string path(PATH_MAX+1,'\0');
    if(::getcwd(&path[0], PATH_MAX) == NULL) {
        DEBUG_PERROR("Failed to getcwd");
        path.clear();
    }
    return path.c_str();
}

std::vector<FileInfo> listdir(const std::string& path, const std::string& sufix, bool recursive, Sort sort) {
    std::vector<FileInfo> ret;
    filewalker(path,recursive,[&](const FileInfo& info) {
        if(sufix.empty() || (info.relpath.find(sufix) != std::string::npos)) {
            ret.push_back(info);
        }
    });
    // Sort
    if (sort == SORT_TIME_ASCENDING) {
        std::sort(ret.begin(), ret.end(),
            [](const FileInfo& a, const FileInfo& b) {
                return a.rawtime < b.rawtime;
            });
    } else if (sort == SORT_TIME_DESCENDING) {
        std::sort(ret.begin(), ret.end(),
            [](const FileInfo& a, const FileInfo& b) {
                return a.rawtime > b.rawtime;
            });
    } else if (sort == SORT_NAME_ASCENDING) {
        std::sort(ret.begin(), ret.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.name < b.name;
        });
    } else if (sort == SORT_NAME_DESCENDING) {
         std::sort(ret.begin(), ret.end(), [](const FileInfo& a, const FileInfo& b) {
            return a.name > b.name;
        });
    }
    return ret;
}

std::vector<FileInfo> listdir(const std::string& path, bool recursive, Sort sort) {
    return listdir(path, "", recursive, sort);
}

bool filewalker(const std::string& path, bool recursive, std::function<void(const FileInfo&)> fn) {

    // Check path type
    if(!isdir(path)) {
        return false;
    }

    // Get full directory path
    std::string abs_dir_path = realpath(path);
    if(abs_dir_path == "") {
        return false;
    }

    // Open directory stream
    DIR* dir = opendir(path.c_str());
    if(dir == NULL) {
        return false;
    }

    // Iterate
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {

        // determinate entry full path
        std::string dname = entry->d_name;
        std::string rel_full_path = joinPath(path,dname);
        std::string abs_full_path = joinPath(abs_dir_path,dname);

        // skip entries "." and ".."
        if (dname == "." || dname == "..") {
            continue;
        }

        // stat for the entry
        struct stat stat_entry;
        stat(rel_full_path.c_str(), &stat_entry);

        // check inode type
        if (S_ISREG(stat_entry.st_mode)) {
            FileInfo info;
            info.name = entry->d_name;
            info.basedir = path;
            info.relpath = rel_full_path;
            info.abspath = abs_full_path;
            info.size = static_cast<uint32_t>(stat_entry.st_size);
            info.rawtime = stat_entry.st_mtime;
            info.timestamp = toJTimestamp(stat_entry.st_mtim);
            
            fn(info);
        } else if(S_ISDIR(stat_entry.st_mode) && recursive) {
            filewalker(rel_full_path,recursive,fn);
        }
    }
    // close handle
    closedir(dir);
    return true;
}

void rmtree(const std::string& path, bool emptyOnly) {

    // Open directory stream
    DIR* dir = opendir(path.c_str());
    if(dir == NULL) {
        return;
    }

    // Iterate
    struct dirent *entry;
    while((entry = readdir(dir)) != NULL) {

        std::string dname = entry->d_name;

        // skip entries "." and ".."
        if (dname == "." || dname == "..") {
            continue;
        }

        // determinate entry full path
        std::string rel_full_path = joinPath(path,dname);
        
        // stat for the entry
        struct stat stat_entry;
        stat(rel_full_path.c_str(), &stat_entry);

        // recurse on dir
        if (S_ISDIR(stat_entry.st_mode)) {
            rmtree(rel_full_path, emptyOnly);
            continue;
        }
        // remove file object
        if(!emptyOnly) {
            rmfile(rel_full_path);
        }
    }
    // remove top dir
    rmdir(path.c_str());
    
    // close handle
    closedir(dir);
}

bool rmfile(const FileInfo& info) {
    return rmfile(info.abspath);
}

bool rmfile(const std::string& path) {
    return unlink(path.c_str()) != -1;
}

namespace time_literals {

const utils::JTimestamp operator "" _YEAR(unsigned long long int y) {
    utils::JTimestamp t; t.year = static_cast<uint16_t>(y); return t;
}
const utils::JTimestamp operator "" _MONTH(unsigned long long int m) {
    utils::JTimestamp t; t.month = static_cast<uint16_t>(m); return t;
}
const utils::JTimestamp operator "" _DAY(unsigned long long int d) {
    utils::JTimestamp t; t.day = static_cast<uint16_t>(d); return t;
}
const utils::JTimestamp operator "" _HOUR(unsigned long long int h) {
    utils::JTimestamp t; t.hour = static_cast<uint16_t>(h); return t;
}
const utils::JTimestamp operator "" _MIN(unsigned long long int mm) {
    utils::JTimestamp t; t.min = static_cast<uint16_t>(mm); return t;
}
const utils::JTimestamp operator "" _SEC(unsigned long long int ss) {
    utils::JTimestamp t; t.sec = static_cast<uint16_t>(ss); return t;
}

} /* end namespace time_literals */

} /* end namespace fs */
