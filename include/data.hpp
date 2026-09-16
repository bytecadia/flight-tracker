#include <SQLiteCpp/SQLiteCpp.h>

#include "strings.hpp"

// TODO: Fix the old member function declaration
inline void lookup_aircraft(SQLite::Database &db)
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
        return;

    mfc = query.getColumn(0).getString();
    mdl = query.getColumn(1).getString();

    int type_aircraft = query.getColumn(2).getInt();
    int type_engine = query.getColumn(3).getInt();
    type = getType(type_aircraft, type_engine);
}

inline std::string lookup_airline(SQLite::Database &db, std::string callsign)
{
    std::string code = parse_chars(callsign);

    if (code.empty())
        return "";

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