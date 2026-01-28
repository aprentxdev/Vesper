# Vesper - local music player
# eteriaal 2025-2026 https://github.com/eteriaal/Vesper

cmake_minimum_required(VERSION 3.28...4.2)
project(Vesper LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenAL REQUIRED)
if(WIN32)
    find_package(FFMPEG REQUIRED COMPONENTS avcodec avformat avutil swresample)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswresample)
endif()

add_library(glad STATIC
    external/glad/src/gl.c
)
target_include_directories(glad PUBLIC external/glad/include)

include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.5
)
FetchContent_MakeAvailable(imgui)

add_library(imgui
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad)

FetchContent_Declare(
    stb_image
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)
FetchContent_MakeAvailable(stb_image)
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${stb_image_SOURCE_DIR})

add_executable(${PROJECT_NAME}
    source/main.cpp
    source/files/files.cpp
    source/files/fonts/loadFonts.cpp
    source/gui/gui.cpp
    source/gui/GuiLoop.cpp
    source/audio/AudioEngine.cpp
    source/metadata/readtags.cpp
    source/metadata/albumArt.cpp
    source/metadata/getlyrics.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    source/core
    source/files
    source/files/fonts
    source/gui
    source/audio
    source/metadata
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    imgui
    CURL::libcurl
    nlohmann_json::nlohmann_json
    stb_image
)

if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS} ${OPENAL_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES} OpenAL::OpenAL)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${FFMPEG_LIBRARIES}
        OpenAL::OpenAL
        pthread dl m
    )
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE X11 Xrandr Xi Xxf86vm Xcursor)
endif()


add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/cacert.pem"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fonts"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts"
)