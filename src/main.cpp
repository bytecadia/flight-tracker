#include <csignal>
#include <functional>
#include <pthread.h>
#include <stop_token>
#include <thread>
#include <vector>
#include <SQLiteCpp/SQLiteCpp.h>

#include "socket.hpp"
#include "queue.hpp"
#include "config.hpp"

int main()
{
    sigset_t s;
    sigemptyset(&s);
    sigaddset(&s, SIGINT);
    sigaddset(&s, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &s, nullptr);

    SQLite::Database db("aircrafts.db", SQLite::OPEN_READONLY);

    Config cfg = parse_cfg("../config.ini");

    TSQueue<std::string> msg_q;
    std::vector<std::jthread> threads;

    std::stop_source stp_src;
    threads.emplace_back(socket_reader, stp_src.get_token(), ref(msg_q), cfg); // config read only
    // threads.emplace_back(proccess, stp_src.get_token());
    // threads.emplace_back(enrich, stp_src.get_token());
    // threads.emplace_back(render, stp_src.get_token());

    int sig;
    sigwait(&s, &sig);

    stp_src.request_stop();
}