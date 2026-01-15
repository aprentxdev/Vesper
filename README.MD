# **Vesper**
**Local music player... with immediate mode gui, ID3 tags, album art, lyrics**

<p align="center">
  <img src="https://img.shields.io/badge/License-MIT-green" alt="License MIT" />
  <img src="https://img.shields.io/badge/C++-20-blue" alt="C++20" />
  <img src="https://img.shields.io/badge/Windows-supported-blue" alt="Windows" />
  <img src="https://img.shields.io/badge/Linux-supported-blue" alt="Linux" />
  <img src="https://img.shields.io/badge/macOS-supported-blue" alt="macOS" />
</p>

<p align="center">
  <img src="https://img.shields.io/badge/CMake-3.28%2B-blue" alt="CMake" />
  <img src="https://img.shields.io/badge/OpenAL-required-orange" alt="OpenAL" />
  <img src="https://img.shields.io/badge/FFmpeg-required-orange" alt="FFmpeg" />
  <img src="https://img.shields.io/badge/ImGui-required-orange" alt="ImGui" />
  <img src="https://img.shields.io/badge/GLEW-required-orange" alt="GLEW" />
  <img src="https://img.shields.io/badge/GLFW-required-orange" alt="GLFW" />
  <img src="https://img.shields.io/badge/OpenGL-required-orange" alt="OpenGL" />
  <img src="https://img.shields.io/badge/JSON-nlohmann-orange" alt="nlohmann_json" />
  <img src="https://img.shields.io/badge/CURL-required-orange" alt="libcurl" />
</p>

<p align="center">
  <img src="Vesper.png" alt="Vesper screenshot" width="800"/>
</p>

> ⚠️ **Work in progress** — the project is functional but unstable. Expect bugs and incomplete features.

---

## Features

- Modern dark interface (ImGui + GLFW)
- Tag parsing and album art display
- Automatic lyrics fetching from [lrclib.net](https://lrclib.net)
- Streaming audio playback (OpenAL + FFmpeg)
- Cross-platform: Windows ⋅ Linux ⋅ macOS


## Planned Features

- **Next:** Real-time spectrum visualizer, drag & drop support, playlists, search  
- **Future:** Settings, EQ

---

## Build Requirements & Setup

Vesper uses CMake and depends on several libraries. Below are instructions for each platform.

### Common
- **CMake** 3.28 – 4.2 (latest recommended)

### Windows (Recommended: MSVC + vcpkg)
1. Install Visual Studio (for MSVC compiler) with "Desktop development with C++" workload.
2. Install **vcpkg**:
  ```bash
  git clone https://github.com/microsoft/vcpkg.git
  ./vcpkg/bootstrap-vcpkg.bat
  ```
3. Integrate vcpkg with VS/CMake
  ```bash
  ./vcpkg integrate install
  ```
4. Install dependencies via vcpkg:
  ```bash
  ./vcpkg install glfw3 glew curl nlohmann-json openal-soft \
      ffmpeg[avcodec,avformat,avutil,swresample] imgui[glfw-binding,opengl3-binding] --triplet x64-windows
  ```
  > Note: imgui is optional — if not installed, it will be fetched automatically from GitHub.
  
5. Build Vesper:
  ```bash
  git clone https://github.com/eteriaal/Vesper.git  
  cd Vesper  
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake  
  cmake --build build --config Release  
  ```
### Linux (Ubuntu/Debian example)
```bash
sudo apt update  
sudo apt install build-essential cmake pkg-config \
    libglfw3-dev libglew-dev libcurl4-openssl-dev nlohmann-json3-dev \
    libopenal-dev libavformat-dev libavcodec-dev libavutil-dev libswresample-dev \
    libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```
For other distros:
- Arch: ``pacman -S base-devel glfw glew curl nlohmann-json openal ffmpeg``

- Fedora: ``dnf install glfw-devel glew-devel curl-devel nlohmann-json-devel openal-soft-devel ffmpeg-devel``  

#### Then build:
```bash
git clone https://github.com/eteriaal/Vesper.git
cd Vesper
cmake -B build -S .
cmake --build build
```

### macOS
```bash
brew install cmake glfw glew curl nlohmann-json openal-soft ffmpeg
```

Build the same way as on Linux.

---

### Runtime Dependencies (for file dialogs)
- **Linux/MacOS**: `zenity`
  ```bash
  sudo apt install zenity    # Ubuntu/Debian
  sudo dnf install zenity    # Fedora
  sudo pacman -S zenity      # Arch
  brew install zenity        # MacOS
  ```
- Windows: No external dependencies (uses native API for file dialogs).
> Note: Planned replace zenity with native cross-platform dialogs (no external tools needed).


---
## Third-Party Fonts
Fonts included in the fonts/ folder:
- **Font Awesome** — MIT License
- **Rubik** — SIL Open Font License (OFL)

Full license texts and required notices are in the 3RDPARTY-LICENSES/ folder.

## License
Vesper is licensed under the MIT License


## Thanks

- To the authors of all used libraries and fonts
- To [lrclib.net](https://lrclib.net) for free lyrics database
- To you — for checking this out

### ★ Found a bug, have a feature idea, or want to contribute? Issues and pull requests welcome!
