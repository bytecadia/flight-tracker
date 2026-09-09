#pragma once

#include "ini.h"
#include <string>

struct Config
{
    std::string host;
    int port;
};

inline Config parse_cfg(std::string path)
{
    mINI::INIFile file(path);
    mINI::INIStructure ini;

    if (!file.read(ini))
    {
        throw std::runtime_error("Failed to read config file: " + path);
    }

    Config config;
    config.host = ini["network"]["host"];
    config.port = std::stoi(ini["network"]["port"]);

    return config;
}