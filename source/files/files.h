#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>

#include "readtags.h"
#include "albumArt.h"

struct AudioMetadata {
    std::string title;
    std::string artist;
    std::string album;
    int year;
    std::string date_str;
    std::string plainLyrics;
    GLuint albumArtTexture = 0;
};

std::string OpenFileDialog();
std::string OpenFolderDialog();
std::string GetExecutableDirectory();
std::string GetResourcePath(const std::string& relative);

std::unordered_map<std::string, AudioMetadata> AddAudioFilesFromDirectory(const std::string& directory);
std::unordered_map<std::string, AudioMetadata> AddAudioFile(const std::string& filePath);

bool IsSupportedAudioFile(const std::filesystem::path& path);