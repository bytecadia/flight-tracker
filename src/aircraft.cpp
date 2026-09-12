#include "aircraft.hpp"

#include <cmath>
#include <numbers>
#include <ranges>
#include <utility>

Aircraft::Aircraft(std::string icao) : icao(icao) {};

std::optional<int> parse_int(const std::string &str)
{
    return !str.empty() ? std::optional<int>(std::stoi(str)) : std::nullopt;
}

std::optional<double> parse_dbl(const std::string &str)
{
    return !str.empty() ? std::optional<double>(std::stod(str)) : std::nullopt;
}

std::optional<std::string> parse_icao(const std::vector<std::string> &msg)
{
    if (msg.size() != 22)
        return std::nullopt;

    if (msg[4].empty())
        return std::nullopt;

    return msg[4];
}

std::optional<bool> Aircraft::parse_msg(const std::vector<std::string> &msg)
{
    last_seen = std::chrono::steady_clock::now();

    if (msg.size() != 22)
        return std::nullopt;

    std::optional<int> msg_type = parse_int(msg[1]);

    switch (parse_int(msg[1]).value_or(-1))
    { // MSG type
    case 1:
        callsign = msg[10];
        break;
    case 2:
        alt = parse_int(msg[11]);
        gs = parse_int(msg[12]);
        trk = parse_int(msg[13]);
        lat = parse_dbl(msg[14]);
        lon = parse_dbl(msg[15]);
        gnd = parse_int(msg[21]);
        break;
    case 3:
        alt = parse_int(msg[11]);
        lat = parse_dbl(msg[14]);
        lon = parse_dbl(msg[15]);
        gnd = parse_int(msg[21]);
        break;
    case 4:
        gs = parse_int(msg[12]);
        trk = parse_int(msg[13]);
        break;
    case 5:
        alt = parse_int(msg[11]);
        gnd = parse_int(msg[21]);
        break;
    case 6:
        alt = parse_int(msg[11]);
        gnd = parse_int(msg[21]);
        break;
    case 7:
        alt = parse_int(msg[11]);
        gnd = parse_int(msg[21]);
        break;
    case 8:
        gnd = parse_int(msg[21]);
        break;
    default:
        return false;
    }
    return true;
}

void Aircraft::lookup(SQLite::Database &db)
{
    // TODO:: What more to add?
    SQLite::Statement query(db,
                            "SELECT"
                            "manufacturer,"
                            "model,"
                            "type_aircraft,"
                            "type_engine"
                            "FROM aircrafts"
                            "WHERE icao = ?;");

    query.bind(1, icao);

    if (!query.executeStep())
        return;

    // mfc = query.getColumn(0).getString();
    // mdl = query.getColumn(1).getString();
    // type = query.getColumn(2).getString();
    // engine = query.getColumn(3).getString();
}

double rads(double deg)
{
    return deg * (std::numbers::pi / 180);
}

double Aircraft::calc_distance(double base_lat, double base_lon)
{
    double phi1 = rads(base_lat);
    double phi2 = rads(*lat);

    double lambda1 = rads(base_lon);
    double lambda2 = rads(*lon);

    double sclr = std::cos((phi1 + phi2) / 2.0);

    double y = phi2 - phi1;
    double x = (lambda2 - lambda1) * sclr;

    return std::sqrt(x * x + y * y) * 3959.0;
}