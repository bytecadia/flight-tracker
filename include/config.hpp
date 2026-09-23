#pragma once

#include <spdlog/spdlog.h>
#include <INIReader.h>
#include <string>

struct Config
{
    std::string host;
    std::string port;

    double lat;
    double lon;

    std::string sml_fnt;
    std::string med_fnt;
    std::string lrg_fnt;
};

inline Config parse_cfg(std::string path)
{
    INIReader reader(path); // TODO: what if reader fails

    if (reader.ParseError() != 0)
        spdlog::error("Failed to parse config '{}' with error code {}", path, reader.ParseError());

    // Logging wrappers for INIReader
    auto get = [&](const std::string &section, const std::string &key, const std::string &def)
    {
        auto v = reader.Get(section, key, def);
        if (v == def && reader.Get(section, key, "") == "")
            spdlog::error("{}.{} missing using default {}", section, key, def);
        return v;
    };

    auto getReal = [&](const std::string &section, const std::string &key, double def)
    {
        auto v = reader.GetReal(section, key, def);
        if (v == def && reader.Get(section, key, "") == "")
            spdlog::error("{}.{} missing using default {}", section, key, std::to_string(def));
        return v;
    };

    Config config;
    config.host = get("network", "host", "127.0.0.1");
    config.port = get("network", "port", "30003");

    config.lat = getReal("Location", "lat", 40.7); // NYC Default
    config.lon = getReal("Location", "lon", -74.0);

    config.sml_fnt = get("font", "sml", "4x6");
    config.med_fnt = get("font", "med", "5x8");
    config.lrg_fnt = get("font", "lrg", "6x10");

    return config;
}