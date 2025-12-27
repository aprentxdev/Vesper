#include "GuiLoop.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}

AudioEngine g_audio;

std::string activeFilePath;
std::string activeFileLyrics;
std::atomic<bool> lyricsLoading(false);
std::atomic<GLuint> activeAlbumArtTexture{0};
std::atomic<bool> albumArtLoading{false};

struct AlbumArtData {
    std::vector<unsigned char> data;
};
std::optional<AlbumArtData> pendingAlbumArt;

void LoadAlbumArtAsync(const std::string& filePath) {
    albumArtLoading = true;
    pendingAlbumArt.reset();

    std::thread([filePath]() {
        AVFormatContext* fmt_ctx = nullptr;
        if (avformat_open_input(&fmt_ctx, filePath.c_str(), nullptr, nullptr) < 0) {
            albumArtLoading = false;
            return;
        }
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            albumArtLoading = false;
            return;
        }

        for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
            AVStream* stream = fmt_ctx->streams[i];
            if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
                AVPacket* pkt = &stream->attached_pic;
                if (pkt->data && pkt->size > 0) {
                    pendingAlbumArt = AlbumArtData{
                        std::vector<unsigned char>(pkt->data, pkt->data + pkt->size)
                    };
                    break;
                }
            }
        }
        avformat_close_input(&fmt_ctx);
        albumArtLoading = false;
    }).detach();
}

static void UpdateCurrentTrackMetadata()
{
    std::string currentPath = g_audio.currentFile();
    if (currentPath.empty()) {
        activeFilePath.clear();
        activeFileLyrics.clear();
        lyricsLoading = false;
        return;
    }
    activeFilePath = currentPath;

    const auto& cache = g_audio.GetMetadataCache();
    auto it = cache.find(currentPath);
    if (it == cache.end()) {
        activeFileLyrics = "No metadata";
        lyricsLoading = false;
        return;
    }

    const AudioMetadata& meta = it->second;

    if (!lyricsLoading.load()) {
        lyricsLoading = true;
        activeFileLyrics.clear();
        std::thread([title = meta.title, artist = meta.artist]() {
            activeFileLyrics = FetchLyrics(title, artist);
            if (activeFileLyrics.empty()) {
                activeFileLyrics = "No lyrics found";
            }
            lyricsLoading = false;
            }).detach();
    }

    LoadAlbumArtAsync(currentPath);
}

void GuiLoop(GLFWwindow* window) {
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        static std::string lastPlayedFile = "";
        std::string currentPlayedFile = g_audio.currentFile();

        if (currentPlayedFile != lastPlayedFile) {
            if (!currentPlayedFile.empty()) {
                UpdateCurrentTrackMetadata();
            }
            else {
                activeFilePath.clear();
                activeFileLyrics.clear();
                GLuint old = activeAlbumArtTexture.load();
                if (old) glDeleteTextures(1, &old);
                activeAlbumArtTexture.store(0);
            }
            lastPlayedFile = currentPlayedFile;
        }


        ImGuiIO& io = ImGui::GetIO();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();

        if (pendingAlbumArt.has_value()) {
            GLuint old = activeAlbumArtTexture.load();
            if (old) glDeleteTextures(1, &old);
            GLuint tex = !pendingAlbumArt->data.empty()
                ? LoadTextureFromMemory(pendingAlbumArt->data.data(), pendingAlbumArt->data.size())
                : 0;
            activeAlbumArtTexture.store(tex);
            pendingAlbumArt.reset();
        }

        ImGui::NewFrame();
        ImGui::PushFont(io.Fonts->Fonts[1]);
        
        ImGui::SetNextWindowSize(ImVec2(600, 350));
        ImGui::SetNextWindowPos(ImVec2(0, 270), ImGuiCond_Always);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        ImGui::Begin("files", nullptr,
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoScrollbar
        );

        ImGui::BeginChild("##Header", ImVec2(0, 45), true, ImGuiWindowFlags_NoScrollbar);
        {
            ImGui::SetCursorPos(ImVec2(12, 8));
            ImGui::PushFont(io.Fonts->Fonts[0]);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 8));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10, 10));

            if (ImGui::Button(u8"\uf15b", ImVec2(40, 30))) {
                std::string file = OpenFileDialog();
                if (!file.empty()) g_audio.AddFile(file);
            }
            ImGui::SameLine();
            if (ImGui::Button(u8"\uf07b", ImVec2(40, 30))) {
                std::string folder = OpenFolderDialog();
                if (!folder.empty()) g_audio.AddFilesFromDirectory(folder);
            }

            ImGui::PopStyleVar(2);
            ImGui::PopFont();
        }
        ImGui::EndChild();

        ImGui::BeginChild("##TrackList", ImVec2(0, 0), false,
            ImGuiWindowFlags_AlwaysVerticalScrollbar |
            ImGuiWindowFlags_HorizontalScrollbar
        );

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2));

        const auto& audioFiles = g_audio.GetAudioFiles();
        const auto& metadataCache = g_audio.GetMetadataCache();

        for (size_t i = 0; i < audioFiles.size(); ++i) {
            const std::string& path = audioFiles[i];
            const auto& meta = metadataCache.at(path);

            std::string display = meta.artist.empty() ? meta.title : meta.artist + " - " + meta.title;
            if (display.empty()) display = std::filesystem::path(path).filename().string();

            bool isPlaying = (activeFilePath == path);

            ImGui::PushID(static_cast<int>(i));

            if (isPlaying) {
                ImVec2 p = ImGui::GetCursorScreenPos();
                ImGui::GetWindowDrawList()->AddRectFilled(
                    p, ImVec2(p.x + ImGui::GetWindowWidth(), p.y + 38),
                    IM_COL32(34, 109, 217, 90), 6.0f);
            }

            if (ImGui::Selectable("##sel", isPlaying, 0, ImVec2(0, 38))) {
                activeFilePath = path;
                g_audio.loadAndPlay(path);  
                UpdateCurrentTrackMetadata();
            }

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 38 + 10);
            ImGui::SetCursorPosX(16);
            ImGui::TextColored(ImVec4(0.70f, 0.70f, 0.75f, 1.0f), "%02d", static_cast<int>(i + 1));

            ImGui::SameLine();
            ImGui::SetCursorPosX(50);
            ImGui::TextColored(isPlaying ? ImVec4(1,1,1,1) : ImVec4(0.92f,0.92f,0.95f,1),
                               "%s", display.c_str());

            ImGui::PopID();
        }

        ImGui::PopStyleVar();
        ImGui::EndChild();
        ImGui::End();
        ImGui::PopStyleVar(2);

        
        ImGui::SetNextWindowSize(ImVec2(300, 620));
        ImGui::SetNextWindowPos(ImVec2(600, 0), ImGuiCond_Always);
        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));
        ImGui::Begin("Now Playing", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);

        ImGui::SetCursorPos(ImVec2(10, 10));
        if (g_RubikLarge) ImGui::PushFont(g_RubikLarge);
        ImGui::TextColored(ImVec4(1,1,1,1), "Now playing:");
        if (g_RubikLarge) ImGui::PopFont();

        ImVec2 AlbumArtSize = ImVec2(250, 250);
        GLuint tex = activeAlbumArtTexture.load();
        if (tex!=0) {
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max = ImVec2(p_min.x + AlbumArtSize.x, p_min.y + AlbumArtSize.y);
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            dl->AddImageRounded((ImTextureID)(intptr_t)tex, p_min, p_max, ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255), 10.0f);
            ImGui::Dummy(AlbumArtSize);
        } else {
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max = ImVec2(p_min.x + AlbumArtSize.x, p_min.y + AlbumArtSize.y);
            ImGui::Dummy(AlbumArtSize);
            ImDrawList* drawList = ImGui::GetForegroundDrawList();
            ImU32 borderColor = IM_COL32(255,255,255,255);
            float borderThickness = 2.0f;
            drawList->AddRect(p_min, p_max, borderColor, 0.0f, 0, borderThickness);
        }

        auto getMeta = [&]() -> const AudioMetadata& {
            static AudioMetadata empty;
            if (activeFilePath.empty()) return empty;
            auto it = metadataCache.find(activeFilePath);
            return it != metadataCache.end() ? it->second : empty;
        };
        const auto& m = getMeta();

        ImGui::PushFont(g_RubikRegular); ImGui::Text("Title:"); ImGui::PopFont();
        ImGui::PushFont(g_RubikMedium); ImGui::TextWrapped("%s", m.title.empty() ? "Unknown" : m.title.c_str()); ImGui::PopFont();
        ImGui::Separator();

        ImGui::PushFont(g_RubikRegular); ImGui::Text("Artist:"); ImGui::PopFont();
        ImGui::PushFont(g_RubikMedium); ImGui::TextWrapped("%s", m.artist.empty() ? "Unknown" : m.artist.c_str()); ImGui::PopFont();
        ImGui::Separator();

        ImGui::PushFont(g_RubikRegular); ImGui::Text("Album:"); ImGui::PopFont();
        ImGui::PushFont(g_RubikMedium); ImGui::TextWrapped("%s", m.album.empty() ? "Unknown" : m.album.c_str()); ImGui::PopFont();
        ImGui::Separator();

        ImGui::PushFont(g_RubikRegular); ImGui::Text("Year:"); ImGui::PopFont();
        ImGui::PushFont(g_RubikMedium); ImGui::Text("%d", m.year); ImGui::PopFont();

        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopFont();

        
        ImGui::SetNextWindowSize(ImVec2(380,620));
        ImGui::SetNextWindowPos(ImVec2(900,0), ImGuiCond_FirstUseEver);
        ImGui::Begin("lyrics", nullptr,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);
        ImGui::SetCursorPos(ImVec2(10, 10));
        ImGui::BeginChild("LyricsScroll", ImVec2(ImGui::GetWindowWidth()-20, ImGui::GetWindowHeight()-20),
                           true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        ImGui::PushFont(g_RubikLarge);
        if (lyricsLoading) {
            ImGui::Text("Loading text from lrclib.net...");
        } else if (!activeFileLyrics.empty()) {
            ImGui::TextWrapped("%s", activeFileLyrics.c_str());
        } else {
            ImGui::Text("Here be lyrics");
        }
        ImGui::PopFont();
        ImGui::EndChild();
        ImGui::End();
        
        
        ImGui::SetNextWindowSize(ImVec2(1280, 100));
        ImGui::SetNextWindowPos(ImVec2(0, 620), ImGuiCond_Always);
        ImGui::PushFont(io.Fonts->Fonts[1]);
        ImGui::SetNextWindowContentSize(ImVec2(1280, 50));

        ImGui::Begin("panel", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar
        );

        ImGui::SetCursorPos(ImVec2(10, 10));

        if (tex) {
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            ImVec2 p_min = ImGui::GetCursorScreenPos();
            ImVec2 p_max = ImVec2(p_min.x + 80, p_min.y + 80);
            dl->AddImageRounded((ImTextureID)(intptr_t)tex, p_min, p_max, ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255), 10.0f);
        } else {
            ImGui::Dummy(ImVec2(80, 80));
            ImDrawList* dl = ImGui::GetForegroundDrawList();
            dl->AddRect(ImVec2(10, 630), ImVec2(90, 710), IM_COL32(255,255,255,255), 0, 0, 2.0f);
        }

        ImGui::SameLine();

        ImGuiStyle& style = ImGui::GetStyle();
        float originalItemSpacingY = style.ItemSpacing.y;
        style.ItemSpacing.y = 0.5f;

        ImGui::BeginGroup();
        ImGui::Dummy(ImVec2(0.0f, 48.f));
        ImGui::SetCursorPosX(98.f);
        std::string activeTitle;
        std::string activeArtist;
        ImGui::Text("%s", m.title.empty() ? "Unknown" : m.title.c_str());
        ImGui::SetCursorPosX(98.f);
        float slideposx2 = ImGui::GetCursorPosX() + 270.f;
        float slideposy2 = ImGui::GetCursorPosY() - 10.f;
        ImGui::Text("%s", m.artist.empty() ? "Unknown" : m.artist.c_str());
        ImGui::EndGroup();

        style.ItemSpacing.y = originalItemSpacingY;
        
        ImGui::SetCursorPosX(98.f);
        float currentTime = static_cast<float>(g_audio.position());
        float trackLength = static_cast<float>(g_audio.duration());

        ImGui::SetCursorPos(ImVec2(slideposx2, slideposy2));
        ImGui::PushItemWidth(600);
        if (ImGui::SliderFloat("##Track Position", &currentTime, 0.0f,
                               trackLength > 0 ? trackLength : 1.0f, "Time: %.1f s")) {
            g_audio.seek(currentTime);
        }
        ImGui::PopItemWidth();
        
        ImGui::SetCursorPos(ImVec2(550, 25));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 10.f));
        ImGui::PushFont(io.Fonts->Fonts[0]);
        
        if (ImGui::Button(u8"\uf048", ImVec2(70, 30))) {
            g_audio.playPrev();
            UpdateCurrentTrackMetadata();
        }

        ImGui::SameLine();
        if (ImGui::Button(g_audio.isPlaying() ? u8"\uf04c" : u8"\uf04b", ImVec2(70, 30))) {
            g_audio.playPause();
        }

        ImGui::SameLine();
        
        if (ImGui::Button(u8"\uf051", ImVec2(70, 30))) {
            g_audio.playNext();
            UpdateCurrentTrackMetadata();
        }
        
        ImGui::SetCursorPos(ImVec2(510, 25));
        bool repeatOneState = g_audio.getRepeatOne();
        ImGui::PushStyleColor(ImGuiCol_Button, repeatOneState ? ImVec4(0.1f, 0.3f, 0.7f, 1) : ImVec4(0.2f, 0.2f, 0.2f, 1));
        if (ImGui::Button(u8"\uf01e", ImVec2(30, 30))) {
            g_audio.setRepeatOne(!repeatOneState);
        }
        ImGui::PopStyleColor();

        ImGui::SetCursorPos(ImVec2(785, 25));
        bool shuffleState = g_audio.getShuffle();
        ImGui::PushStyleColor(ImGuiCol_Button, shuffleState ? ImVec4(0.1f, 0.3f, 0.7f, 1) : ImVec4(0.2f, 0.2f, 0.2f, 1));
        if (ImGui::Button(u8"\uf074", ImVec2(30, 30))) { 
            g_audio.setShuffle(!shuffleState);
        }
        ImGui::PopStyleColor();

        ImGui::PopFont();
        ImGui::PopStyleVar();

        float volume = g_audio.volume();
        ImGui::SameLine();
        ImGui::SetCursorPosX(slideposx2 + 655.f);
        ImGui::SetCursorPosY(slideposy2 + 1.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 4.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 8.0f);
        ImGui::PushItemWidth(150);
        if (ImGui::SliderFloat("##Volume", &volume, 0.0f, 1.0f, "")) {
            g_audio.setVolume(volume);
        }
        ImGui::PopItemWidth();
        ImGui::PopStyleVar(3);

        ImGui::PopFont();
        ImGui::End();

        
        ImGui::SetNextWindowSize(ImVec2(600, 270), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
        ImGui::Begin("Visualizer", nullptr,
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar);

        // to be implemented

        ImGui::End();
        ImGui::PopFont();

        ImGui::Render();
        glClearColor(0.110f, 0.110f, 0.125f, 1.000f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}