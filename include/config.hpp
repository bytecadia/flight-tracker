#pragma once

// #include "ini.h" TODO: Fix package includes
#include <INIReader.h>
#include <string>

struct Config
{
    std::string host;
    std::string port;

    double lat;
    double lon;
};

inline Config parse_cfg(std::string path)
{
    INIReader reader(path); // TODO: what if reader fails

    Config config;
    config.host = reader.Get("network", "host", "127.0.0.1");
    config.port = reader.Get("network", "port", "30003");

    config.lat = reader.GetReal("Location", "lat", 40.7); // NYC Default
    config.lon = reader.GetReal("Location", "lon", -74.0);

    return config;
}