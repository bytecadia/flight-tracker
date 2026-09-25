#pragma once

#include <stop_token>
#include <string>
#include <vector>

#include <SQLiteCpp/SQLiteCpp.h>

#include "config.hpp"
#include "queue.hpp"
#include "snapshot.hpp"

void process(std::stop_token st,
             TSQueue<std::string> &msg_q,
             Snapshot &snapshot,
             SQLite::Database &db,
             const Config &cfg);
