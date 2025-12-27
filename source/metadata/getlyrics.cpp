// TO-DO: replace libcurl with cpr
// TO-DO: synced lyrics
// TO-DO: cache lyrics
// TO-DO: add timeout

#include "getlyrics.h"
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

// cert path
std::string exeDir = GetExecutableDirectory();
std::string caPath = exeDir + 
#ifdef _WIN32
    "\\cacert.pem";
#else
    "/cacert.pem";
#endif

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string FetchLyrics(const std::string& title, const std::string& artist) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string readBuffer;

    char* escTitle = curl_easy_escape(curl, title.c_str(), 0);
    char* escArtist = curl_easy_escape(curl, artist.c_str(), 0);

    std::string url = std::string("https://lrclib.net/api/search?track_name=") +
                      escTitle + "&artist_name=" + escArtist;

    curl_free(escTitle);
    curl_free(escArtist);

    curl_easy_setopt(curl, CURLOPT_CAINFO, caPath.c_str());
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Vesper (https://github.com/eteriaal/Vesper)");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "Curl error: " << curl_easy_strerror(res) << std::endl;
        return "";
    }

    try {
        auto j = json::parse(readBuffer);
        if (j.is_array() && !j.empty()) {
            return j[0].value("plainLyrics", "");
        }
    } catch (json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
    }

    return "";
}
