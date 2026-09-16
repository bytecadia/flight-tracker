#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

std::optional<std::string> parse_icao(const std::vector<std::string> &msg);
double rads(double deg);

struct Aircraft
{
    enum class Type
    {
        Prop,
        Jet,
        Heli,
        Unk
    };

    // SBS Data - Required
    std::string icao; // Field 4 - all messages

    // SBS Data - Optional
    std::string callsign;      // Field 11 - MSG 1
    std::optional<int> alt;    // Field 12 - MSG 2,3,5,6,7
    std::optional<int> gs;     // Field 13 - MSG 2,4
    std::optional<int> trk;    // Field 14 - MSG 2,4
    std::optional<double> lat; // Field 15 - MSG 2,3
    std::optional<double> lon; // Field 16 - MSG 2,3
    std::optional<bool> gnd;   // Field 22 - MSG 2,3,5,6,7,8

    // Database Data
    bool processed;
    std::string mfc;
    std::string mdl;
    // TODO: Should airline be here

    // Derived Data
    // Operation op = Operation::Non;
    double dist = 0.0;
    Type type = Type::Unk;

    std::chrono::steady_clock::time_point last_seen;

    // Constructor that takes in just the ICAO and defaults the other fields
    explicit Aircraft(std::string icao);

    // Methods TODO: Do these make sense
    std::optional<bool> parse_msg(const std::vector<std::string> &msg);
    void lookup_aircraft(SQLite::Database &db);
    double calc_dist(double base_lat, double base_lon);
};
