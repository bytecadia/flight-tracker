#pragma once

#include <stop_token>
#include "queue.hpp"
#include "config.hpp"

void socket_reader(std::stop_token st, TSQueue<std::string> &q, const Config &cfg);
