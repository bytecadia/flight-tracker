#include <stop_token>

#include "config.hpp"
#include "snapshot.hpp"

void render(std::stop_token st, Snapshot &snap, const Config &cfg, SQLite::Database &db);