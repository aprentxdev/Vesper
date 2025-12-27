// TO-DO: replace win32api/zenity with nfd (crossplatform)

#include "files.h"
#include <iostream>
#include <algorithm>
#include <set>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <shobjidl.h>
    #include <commdlg.h>
    #include <codecvt>
#else
    #include <cstdlib>
    #include <unistd.h>
    #include <limits.h>
#endif

#ifdef __APPLE__
    #include <mach-o/dyld.h>
#endif

bool IsSupportedAudioFile(const std::filesystem::path& path) {
    static const std::set<std::string> supportedExtensions = {
        ".mp3", ".wav", ".flac", ".m4a", ".ogg", ".aac", ".opus"
    };
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return supportedExtensions.find(ext) != supportedExtensions.end();
}

std::string GetExecutableDirectory()
{
    std::string exePath;

#ifdef _WIN32
    char path[MAX_PATH];
    HMODULE hm = nullptr;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCSTR)&GetExecutableDirectory, &hm))
    {
        GetModuleFileNameA(hm, path, MAX_PATH);
        exePath = path;
    }

#elif __APPLE__
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        exePath = path;
    }

#elif __linux__
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    if (count != -1)
    {
        path[count] = '\0';
        exePath = path;
    }
#endif

    if (exePath.empty())
    {
        // Fallback: current directory
        return std::filesystem::current_path().string();
    }

    return std::filesystem::path(exePath).parent_path().string();
}

std::string GetResourcePath(const std::string& relative) {
    return GetExecutableDirectory() + "/" + relative;
}


std::string OpenFileDialog() {
#ifdef _WIN32
    wchar_t filePath[MAX_PATH] = {0};
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"All Audio Files (*.mp3;*.wav;*.flac;*.ogg;*.aac;*.m4a;*.opus)\0*.mp3;*.wav;*.flac;*.ogg;*.aac;*.m4a;*.opus\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(filePath);
    }
    return "";
#else
    // use zenity
    char command[512];
    snprintf(command, sizeof(command),
             "zenity --file-selection --title=\"Select Audio File\" "
             "--file-filter=\"Audio files | *.mp3 *.wav *.flac *.ogg *.aac *.m4a *.opus\" 2>/dev/null");

    FILE* fp = popen(command, "r");
    if (!fp) return "";

    char result[4096] = {0};
    if (fgets(result, sizeof(result)-1, fp)) {
        std::string path = result;
        path.erase(path.find_last_not_of("\n\r") + 1);
        pclose(fp);
        if (!path.empty() && std::filesystem::exists(path))
            return path;
    }
    pclose(fp);
    return "";
#endif
}

std::string OpenFolderDialog() {
#ifdef _WIN32
    std::string folderPath;
    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr)) {
        pfd->SetOptions(FOS_PICKFOLDERS);
        if (SUCCEEDED(pfd->Show(nullptr))) {
            IShellItem* psi;
            if (SUCCEEDED(pfd->GetResult(&psi))) {
                PWSTR pszPath;
                if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
                    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
                    folderPath = conv.to_bytes(pszPath);
                    CoTaskMemFree(pszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }
    return folderPath;
#else
    // use zenity
    char command[512];
    snprintf(command, sizeof(command), "zenity --file-selection --directory --title=\"Select Music Folder\" 2>/dev/null");
    FILE* fp = popen(command, "r");
    if (!fp) return "";

    char result[4096] = {0};
    if (fgets(result, sizeof(result)-1, fp)) {
        std::string path = result;
        path.erase(path.find_last_not_of("\n\r") + 1);
        pclose(fp);
        if (!path.empty() && std::filesystem::is_directory(path))
            return path;
    }
    pclose(fp);
    return "";
#endif
}


// Scans a directory, extracts tags from all supported audio files.
// Returns a map: full path -> metadata.
std::unordered_map<std::string, AudioMetadata> AddAudioFilesFromDirectory(const std::string& directory) {
    std::unordered_map<std::string, AudioMetadata> metadataMap;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (entry.is_regular_file() && IsSupportedAudioFile(entry.path())) {
                std::string path = entry.path().u8string();
                if (metadataMap.find(path) == metadataMap.end()) {
                    std::string title, artist, album, date_str;
                    int year;
                    ReadAudioTags(path.c_str(), &title, &artist, &album, &year, &date_str);
                    metadataMap[path] = {title, artist, album, year, date_str};
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory: " << e.what() << std::endl;
    }
    return metadataMap;
}

// Loads metadata from a single audio file.
// Returns a map with one entry: path -> metadata.
std::unordered_map<std::string, AudioMetadata> AddAudioFile(const std::string& filePath) {
    std::unordered_map<std::string, AudioMetadata> metadataMap;
    std::filesystem::path p(filePath);
    if (std::filesystem::is_regular_file(p) && IsSupportedAudioFile(p)) {
        std::string pathStr = p.u8string();
        std::string title, artist, album, date_str;
        int year;
        ReadAudioTags(pathStr.c_str(), &title, &artist, &album, &year, &date_str);
        metadataMap[pathStr] = {title, artist, album, year, date_str};
    }
    return metadataMap;
}