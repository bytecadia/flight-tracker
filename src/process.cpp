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
             TSQueue<std::vector<std::string>> &enrich_q,
             TSQueue<std::vector<std::string>> &result_q,
             Snapshot &snapshot,
             SQLite::Database &db,
             const Config &cfg)
{
    std::map<std::string, Aircraft> aircrafts;
    auto deadline = std::chrono::steady_clock::now() + 30s;

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
            if (!icao.empty())
            {
                auto [it, inserted] = aircrafts.try_emplace(icao, icao);

                if (it->second.parse_msg(fields))
                {
                    it->second.last_seen = std::chrono::steady_clock::now();
                }
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
            if (aircrafts.empty()) // TODO: What should UI show here?
                continue;

            // Select featured aircraft
            Aircraft *featured = nullptr;
            double closest = 0;
            for (auto &[icao, a] : aircrafts)
            {
                if (!a.lat || !a.lon)
                    continue;

                double distance = calc_dist(cfg.lat, cfg.lon, *a.lat, *a.lon);
                if (!featured || distance < closest)
                {
                    closest = distance;
                    featured = &a;
                }
            }
            if (featured)
                snapshot.write(*featured);
        }
    }
}