/*
 *  post_response.cpp
 *
 *  Created on: Mar 16, 2021
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  HTTP POST response
 *
 */

//==============================================================================
// INCLUDES
//==============================================================================
#include "config/config_parser.h"

#include <fstream>

//==============================================================================
// METHODS
//==============================================================================
Config::Config()
{
    std::ifstream mConfig("/etc/config.json");
    mConfig >> configuration;
}

const nlohmann::json& Config::get_config() { return getInstance().configuration; }
