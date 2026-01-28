// TO-DO: synced lyrics
// TO-DO: cache lyrics

#include "getlyrics.h"
#include <string>
#include <filesystem>

using json = nlohmann::json;

std::optional<std::string> getLyrics(
    const std::string& artist,
    const std::string& title,
    const std::string& userAgent
)
{
    cpr::Parameters params{
        {"artist_name", artist},
        {"track_name",  title}
    };

    cpr::Header headers{
        {"User-Agent", userAgent}
    };

    auto r = cpr::Get(
        cpr::Url{"https://lrclib.net/api/search"},
        std::move(params),
        std::move(headers),
        cpr::Timeout{10000}
    );

    if (r.status_code != 200 || r.text.empty()) {
        return std::nullopt;
    }

    try
    {
        auto j = json::parse(r.text);

        if (!j.is_array() || j.empty()) {
            return std::nullopt;
        }

        const auto& best = j[0];

        if (best.contains("plainLyrics") && best["plainLyrics"].is_string()) {
            return best["plainLyrics"].get<std::string>();
        }

        return std::nullopt;
    }
    catch (const json::exception&)
    {
        return std::nullopt;
    }
}