# **Vesper**
**Local music player with metadata, lyrics, and spectrum visualization**

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

![Vesper screenshot](Vesper.png)

> ⚠️ Unstable version — may contain bugs or incomplete features.

---

## Features

- Modern dark interface (ImGui + GLFW)
- Tag and album art parsing
- Automatic lyrics fetching via lrclib.net
- Streaming audio playback (OpenAL + FFmpeg)
- Cross-platform: Windows ⋅ Linux ⋅ macOS

---

## Planned Features

- **Next:** Real-time spectrum visualizer, drag & drop, playlists, search  
- **Future:** Settings, EQ

---

## Build 
```bash
git clone https://github.com/eteriaal/Vesper.git
cd Vesper
cmake -B build
cmake --build build
```

## Third-Party libraries
Dependencies are located via CMake find_package:
```cmake
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenAL REQUIRED)
find_package(ImGui REQUIRED)
```

## Third-Party Fonts
Fonts included in the fonts/ folder:
- **Font Awesome** — MIT License
- **Rubik** — SIL Open Font License (OFL)

Full license texts and required notices are in the 3RDPARTY-LICENSES/ folder.

## License
Vesper is licensed under the MIT License


## Thanks

- To the authors of all libraries and fonts used in the project
- To lrclib.net for free lyrics database
- To you — for checking this out

### ★ Found a bug or idea? Issues and pull requests welcome!