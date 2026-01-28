#pragma once

#include <string>
#include <optional>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

std::optional<std::string> getLyrics(
    const std::string& artist,
    const std::string& title,
    const std::string& userAgent = "Vesper[](https://github.com/eteriaal/Vesper)"
);