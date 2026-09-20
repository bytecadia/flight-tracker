#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>

#include "strings.hpp"
#include "parser.hpp"
// TODO: Bad to just be including this for the AircraftType?

struct AircraftInfo
{
    std::string mfc;
    std::string mdl;
    AircraftType type;
};

// TODO: Should this be returning optional, does it make sense for partials here
inline AircraftInfo lookup_aircraft(SQLite::Database &db, std::string icao)
{
    try
    {
        // TODO:: What more to add?
        SQLite::Statement query(db,
                                "SELECT "
                                "manufacturer, "
                                "model, "
                                "type_aircraft, "
                                "type_engine "
                                "FROM aircrafts "
                                "WHERE icao = ?;");

        query.bind(1, icao);

        if (!query.executeStep())
            return AircraftInfo{};

        AircraftInfo info;
        info.mfc = query.getColumn(0).getString();
        info.mdl = query.getColumn(1).getString();

        int type_aircraft = query.getColumn(2).getInt();
        int type_engine = query.getColumn(3).getInt();

        info.type = get_type(type_aircraft, type_engine);

        return info;
    }
    catch (const std::exception &e)
    {
        spdlog::error("Lookup aircraft failed for ICAO: '{}' with error '{}'",
                      icao, e.what());
    }

    return AircraftInfo{}; // TODO: Hacking here see todo above
}

inline std::string lookup_airline(SQLite::Database &db, std::string callsign)
{
    std::string code = parse_chars(callsign);

    if (code.empty())
        return "";

    try
    {
        SQLite::Statement query(db,
                                "SELECT "
                                "name "
                                "FROM airlines "
                                "WHERE icao = ?;");

        query.bind(1, code);

        if (!query.executeStep())
            return "";

        return query.getColumn(0).getString();
    }
    catch (const SQLite::Exception &e)
    {
        spdlog::error("Lookup airline failed for callsign '{}' and ICAO '{}' with error '{}'", callsign, code, e.what());
    }
    return "";
}