#include <map>
#include <vector>
#include <chrono>
#include <string>
#include <stop_token>
#include <ranges>

#include "ts_queue.hpp"
#include "aircraft.hpp"
#include "snapshot.hpp"

using namespace std::chrono_literals;

std::vector<std::string> split(const std::string &msg)
{
    return msg | std::views::split(',') | std::ranges::to<std::vector<std::string>>();
}

void process(std::stop_token st,
             TSQueue<std::string> &msg_q,
             TSQueue<std::vector<std::string>> &enrich_q,
             TSQueue<std::vector<std::string>> &result_q,
             Snapshot snapshot,
             SQLite::Database db)
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

            std::optional<std::string> icao = parse_icao(fields);
            if (icao)
            {
                auto [it, inserted] = aircrafts.try_emplace(*icao, *icao);

                it->second.parse_msg(fields);
                it->second.lookup(db);
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

            if (aircrafts.empty())
                return;

            // Select featured aircraft
            Aircraft *featured = &(aircrafts.begin()->second);
            double closest = featured->distance;
            for (auto &[icao, a] : aircrafts)
            {
                if (a.distance < closest)
                {
                    closest = a.distance;
                    featured = &a;
                }
            }
            snapshot.write(*featured);
            deadline = now + 30s;
        }
    }
}
