#include <string>
#include <vector>

enum class AircraftType
{
    Prop,
    Jet,
    Heli,
    Unk
};

// Caller must check if icao is not empty
inline std::string parse_icao(const std::vector<std::string> &msg)
{
    // TODO: Do I need more checks here?
    if (msg.size() != 22)
        return "";

    return msg[4];
}

inline AircraftType get_type(int type, int engine)
{
    if (type == 6) // Rotocraft
        return AircraftType::Heli;

    if (type == 4 || // Fixed-wing, singe-engine
        type == 5)   // Fixed-wing, multi-engine
    {
        if (engine == 4 || // Turbojet
            engine == 5)   // Turbofan
            return AircraftType::Jet;

        if (engine == 1 ||  // Reciprocating
            engine == 2 ||  // Turboprop
            engine == 7 ||  // 2-cycle
            engine == 8 ||  // 4-cycle
            engine == 10 || // Electric
            engine == 11)   // Rotary
            return AircraftType::Prop;
    }

    return AircraftType::Unk;
}