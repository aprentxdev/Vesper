#include <GL/glew.h>        
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <codecvt>
#include <locale>
#include <regex>
#include <algorithm>
#include <cctype>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
}

using std::string;

void ReadAudioTags(const char* filename, std::string* title, std::string* artist, std::string* album, int* year, std::string* date_str = nullptr) {
    *title = "Unknown Title";
    *artist = "Unknown Artist";
    *album = "Unknown Album";
    *year = 0;
    if (date_str) *date_str = "";

    AVFormatContext* fmt_ctx = nullptr;

    if (avformat_open_input(&fmt_ctx, filename, nullptr, nullptr) < 0) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return;
    }

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream info: " << filename << std::endl;
        avformat_close_input(&fmt_ctx);
        return;
    }

    AVDictionary* metadata = fmt_ctx->metadata;
    if (!metadata) {
        std::cerr << "No metadata found in file: " << filename << std::endl;
        avformat_close_input(&fmt_ctx);
        return;
    }

    AVDictionaryEntry* tag = nullptr;

    while ((tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
        std::string key = tag->key ? tag->key : "";
        std::string value = tag->value ? tag->value : "";

        // Normalize key to lowercase
        std::string key_lower = key;
        std::transform(key_lower.begin(), key_lower.end(), key_lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        // Recognize standard metadata keys
        if (key_lower == "title") {
            *title = value;
        }
        else if (key_lower == "artist") {
            *artist = value;
        }
        else if (key_lower == "album") {
            *album = value;
        }
        else if (key_lower == "date" || key_lower == "year") {
            if (date_str) *date_str = value; // if date found

            try { // year search
                std::smatch m;
                std::regex year_regex(R"((\d{4}))");
                if (std::regex_search(value, m, year_regex)) {
                    *year = std::stoi(m[1].str());
                }
                else {
                    *year = 0;
                }
            }
            catch (...) {
                *year = 0;
            }
        }
    }

    avformat_close_input(&fmt_ctx);

    // debug log
    std::cerr << "Tags read successfully: Title=" << *title << ", Artist=" << *artist
        << ", Album=" << *album << ", Year=" << *year;
    if (date_str) {
        std::cerr << ", Date=" << *date_str;
    }
    std::cerr << std::endl;
}

