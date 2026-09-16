#include <ranges>
#include <utility>

#include "aircraft.hpp"
#include "strings.hpp"

Aircraft::Aircraft(std::string icao) : icao(icao) {};

std::optional<bool> Aircraft::parse_msg(const std::vector<std::string> &msg)
{
    if (msg.size() != 22) // TODO: More checks for robustness?
        return std::nullopt;

    // TODO: Should these not change the value of field if parsing failed
    //  Example of check `if (auto v = parse_num<int>(msg[11])) alt = v;`
    switch (parse_num<int>(msg[1]).value_or(-1))
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
        return false;
    }
    return true;
}
