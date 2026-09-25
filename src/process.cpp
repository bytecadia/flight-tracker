#include <map>
#include <vector>
#include <chrono>
#include <string>
#include <stop_token>
#include <ranges>
#include <SQLiteCpp/SQLiteCpp.h>

#include "queue.hpp"
#include "aircraft.hpp"
#include "snapshot.hpp"
#include "strings.hpp"
#include "config.hpp"
#include "geometry.hpp"
#include "process.hpp"
#include "data.hpp"

using namespace std::chrono_literals;

void process(std::stop_token st,
             TSQueue<std::string> &msg_q,
             Snapshot &snapshot,
             SQLite::Database &db,
             const Config &cfg)
{
    std::map<std::string, Aircraft> aircrafts;
    auto deadline = std::chrono::steady_clock::now() + 30s;

    long dbg_msgs = 0, dbg_bad_icao = 0;                                 // DBG
    spdlog::debug("process: started");                                   // DBG

    while (!st.stop_requested())
    {
        std::optional<std::string> msg = msg_q.pop_until(st, deadline);
        if (st.stop_requested())
            return;

        // Process message
        if (msg)
        {
            std::vector<std::string> fields = split(*msg);

            std::string icao = parse_icao(fields);
            if (++dbg_msgs <= 3)                                                     // DBG
                spdlog::debug("process: msg#{} {} fields, icao='{}' raw='{}'",       // DBG
                              dbg_msgs, fields.size(), icao, *msg);                  // DBG
            if (icao.empty())                                                        // DBG
                ++dbg_bad_icao;                                                      // DBG

            if (!icao.empty())
            {
                auto [it, inserted] = aircrafts.try_emplace(icao, icao);
                it->second.parse_msg(fields);
                it->second.last_seen = std::chrono::steady_clock::now();
            }
        }

        // Maintenance
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline)
        {
            // Clean stale aircraft
            for (auto it = aircrafts.begin(); it != aircrafts.end();)
            {
                if (now - it->second.last_seen >= 60s)
                    it = aircrafts.erase(it);
                else
                    ++it;
            }

            deadline = now + 30s;
            spdlog::debug("process: tick - {} msgs seen ({} with no icao), {} aircraft tracked",  // DBG
                          dbg_msgs, dbg_bad_icao, aircrafts.size());                              // DBG
            if (aircrafts.empty()) // TODO: What should UI show here?
            {
                spdlog::debug("process: no aircraft tracked, nothing to show");   // DBG
                continue;
            }

            // Select featured aircraft
            Aircraft *featured = nullptr;
            double closest = 0;
            int dbg_with_pos = 0;                                                 // DBG
            for (auto &[icao, a] : aircrafts)
            {
                if (!a.lat || !a.lon)
                    continue;
                ++dbg_with_pos;                                                   // DBG

                double distance = calc_dist(cfg.lat, cfg.lon, *a.lat, *a.lon);
                if (!featured || distance < closest)
                {
                    closest = distance;
                    featured = &a;
                }
            }
            spdlog::debug("process: {}/{} aircraft have a position",              // DBG
                          dbg_with_pos, aircrafts.size());                        // DBG
            if (featured)
                spdlog::debug("process: featured={} at {:.1f} mi, alt={} gs={} cs='{}' -> snapshot",  // DBG
                              featured->icao, closest, featured->alt.value_or(-1),                    // DBG
                              featured->gs.value_or(-1), featured->callsign);                         // DBG
            else
                spdlog::debug("process: NO aircraft has lat/lon, snapshot not written"); // DBG

            if (featured)
                snapshot.write(*featured);
        }
    }
}