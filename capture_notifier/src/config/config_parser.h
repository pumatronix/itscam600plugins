/*
 *  post_response.h
 *
 *  Created on: Mar 16, 2021
 *      Author: Eduardo Almeida - Pumatronix
 *
 *  Configuration parser
 *
 */
#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

//==============================================================================
// INCLUDES
//==============================================================================
#include <nlohmann/json.hpp>

//==============================================================================
// CLASS
//==============================================================================

/**
 * @brief Log
 * This class handles the config.json file
 */
class Config {
private:
    static Config& getInstance()
    {
        static Config instance;
        return instance;
    }

    nlohmann::json configuration;

    Config();

public:
    static const nlohmann::json& get_config();
};

#endif /* __CONFIG_PARSER_H__ */
