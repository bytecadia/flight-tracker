#include <ranges>
#include <utility>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ranges.h>

#include "aircraft.hpp"
#include "strings.hpp"

Aircraft::Aircraft(std::string icao) : icao(icao) {};

bool Aircraft::parse_msg(const std::vector<std::string> &msg)
{
    if (msg.size() != 22) // TODO: More checks for robustness?
    {
        spdlog::error("Failed to parse message (wrong size) with message '{}'", fmt::join(msg, ", "));
        return false;
    }

    // TODO: Should these not change the value of field if parsing failed
    // Example of check `if (auto v = parse_num<int>(msg[11])) alt = v;`
    int type = parse_num<int>(msg[1]).value_or(-1);
    switch (type)
    { // MSG type
    case 1:
        callsign = msg[10];
        break;
    case 2:
        alt = parse_num<int>(msg[11]);
        gs = parse_num<int>(msg[12]);
        trk = parse_num<int>(msg[13]);
        lat = parse_num<double>(msg[14]);
        lon = parse_num<double>(msg[15]);
        gnd = parse_num<int>(msg[21]);
        break;
    case 3:
        alt = parse_num<int>(msg[11]);
        lat = parse_num<double>(msg[14]);
        lon = parse_num<double>(msg[15]);
        gnd = parse_num<int>(msg[21]);
        break;
    case 4:
        gs = parse_num<int>(msg[12]);
        trk = parse_num<int>(msg[13]);
        break;
    case 5:
        alt = parse_num<int>(msg[11]);
        gnd = parse_num<int>(msg[21]);
        break;
    case 6:
        alt = parse_num<int>(msg[11]);
        gnd = parse_num<int>(msg[21]);
        break;
    case 7:
        alt = parse_num<int>(msg[11]);
        gnd = parse_num<int>(msg[21]);
        break;
    case 8:
        gnd = parse_num<int>(msg[21]);
        break;
    default:
        spdlog::error("Failed to parse message (invalid message type) with type '{}'", type);
        return false;
    }
    return true;
}
