#pragma once

#include <iostream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include "files.h"

std::string FetchLyrics(const std::string& title, const std::string& artist);