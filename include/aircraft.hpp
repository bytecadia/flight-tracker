#pragma once

#include <chrono>
#include <optional>
#include <string>

struct Aircraft
{
    // SBS Data - Required
    std::string icao; // Field 4 - all messages
    std::chrono::steady_clock::time_point last_seen;

    // SBS Data - Optional
    std::string callsign;      // Field 11 - MSG 1
    std::optional<int> alt;    // Field 12 - MSG 2,3,5,6,7
    std::optional<int> gs;     // Field 13 - MSG 2,4
    std::optional<int> trk;    // Field 14 - MSG 2,4
    std::optional<double> lat; // Field 15 - MSG 2,3
    std::optional<double> lon; // Field 16 - MSG 2,3
    std::optional<int> gnd;    // Field 22 - MSG 2,3,5,6,7,8

    // Constructor that takes in just the ICAO and defaults the other fields
    explicit Aircraft(std::string icao);

    bool parse_msg(const std::vector<std::string> &msg);
};
