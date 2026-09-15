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
    // enum class Operation
    // {
    //     Com,
    //     Non
    // };

    // enum class Type
    // {
    //     Prop,
    //     Jet,
    //     Heli
    // };

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

    // Derived Data
    bool has_logo = false;
    double dist = 0.0;

    // Database Data
    bool looked_up;
    std::string mfc;
    std::string mdl;
    std::string type;
    std::string eng;

    // Operation op = Operation::Non;
    // Type type = Type::Prop;

    std::chrono::steady_clock::time_point last_seen;

    // Constructor that takes in just the ICAO and defaults the other fields
    explicit Aircraft(std::string icao);

    // Methods
    std::optional<bool> parse_msg(const std::vector<std::string> &msg);
    void lookup(SQLite::Database &db);
    double calc_dist(double base_lat, double base_lon);
};
