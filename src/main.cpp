#include <csignal>
#include <functional>
#include <pthread.h>
#include <stop_token>
#include <thread>
#include <vector>
#include <SQLiteCpp/SQLiteCpp.h>
#include <spdlog/spdlog.h>

#include "socket.hpp"
#include "queue.hpp"
#include "config.hpp"
#include "process.hpp"
#include "snapshot.hpp"
#include "render.hpp"

int main()
{
    spdlog::set_level(spdlog::level::debug);                              // DBG
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%L%$] [t:%t] %v");              // DBG
    spdlog::debug("main: starting");                                      // DBG

    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    sigaddset(&s, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &s, nullptr);

    std::optional<SQLite::Database> db;
    try
    {
        db.emplace("aircraft.db", SQLite::OPEN_READONLY);
    }
    catch (const SQLite::Exception &e)
    {
        spdlog::error("Database 'aircraft.db' failed to open with error {}", e.what()); // Hardcoded for now
    }

    if (!db)
        return 0;

    Config cfg = parse_cfg(CONFIG_PATH);
    spdlog::debug("cfg: feed={}:{} obs=({},{})", cfg.host, cfg.port, cfg.lat, cfg.lon);   // DBG
    spdlog::debug("cfg: fonts sml={} med={} lrg={}", cfg.sml_fnt, cfg.med_fnt, cfg.lrg_fnt); // DBG
    spdlog::debug("cfg: panel {}x{} pad={} row_gap={} img_span={} img_h={}",              // DBG
                  cfg.cols, cfg.rows, cfg.padding, cfg.row_gap, cfg.img_span, cfg.img_h); // DBG

    TSQueue<std::string> msg_q;
    Snapshot snapshot;

    std::vector<std::jthread> threads;

    std::stop_source stp_src;
    threads.emplace_back(socket_reader, stp_src.get_token(), std::ref(msg_q), cfg); // config read only
    threads.emplace_back(process, stp_src.get_token(), std::ref(msg_q), std::ref(snapshot), std::ref(*db), std::cref(cfg));
    threads.emplace_back(render, stp_src.get_token(), std::ref(snapshot), std::cref(cfg), std::ref(*db));
    spdlog::debug("main: 3 threads started, waiting for signal");        // DBG

    int sig;
    sigwait(&s, &sig);
    spdlog::debug("main: got signal {}, stopping", sig);                 // DBG

    stp_src.request_stop();
}