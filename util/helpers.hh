#pragma once

#include "parser.hh"

#include <numeric>
#include <ranges>
#include <string>
#include <vector>

// Pretty-print a string (escaping unprintable characters, and with a maximum length)
std::string pretty_print( std::string_view str, size_t max_length = 32 );
