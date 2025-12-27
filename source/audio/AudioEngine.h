#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <optional>
#include <vector>
#include <queue>
#include <condition_variable>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
}

#include <al.h>
#include <alc.h>
#include "files.h"

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    void loadAndPlay(const std::string& filePath);
    void play();
    void pause();
    void playPause();
    void stop();
    void seek(double seconds);
    void setVolume(float v);

    void playNext();
    void playPrev();

    void setRepeatOne(bool enabled) { m_repeatOne.store(enabled); }
    void setShuffle(bool enabled);

    bool getRepeatOne() const { return m_repeatOne.load(); }
    bool getShuffle() const { return m_shuffle.load(); }

    bool isPlaying() const { return m_playing.load(); }
    double position() const { return m_position.load(); }
    double duration() const { return m_duration.load(); }
    float volume() const { return m_volume.load(); }
    std::string currentFile() const;
    std::optional<AudioMetadata> currentMetadata() const;

    void AddFilesFromDirectory(const std::string& directory);
    void AddFile(const std::string& filePath);
    const std::vector<std::string>& GetAudioFiles() const;
    const std::unordered_map<std::string, AudioMetadata>& GetMetadataCache() const;

private:
    static constexpr int NUM_BUFFERS = 4;
    static constexpr size_t BUFFER_SAMPLES = 8192;
    static constexpr size_t FFT_SIZE = 2048;

    void workerThread();
    bool openFile(const std::string& path);
    int decodeNextBlock(int16_t* outBuffer, int maxSamples);
    ALenum formatFromChannels(int channels);

    ALCdevice* m_device{nullptr};
    ALCcontext* m_context{nullptr};
    ALuint m_source{0};
    ALuint m_buffers[NUM_BUFFERS]{0};

    AVFormatContext* m_fmt{nullptr};
    AVCodecContext* m_codec{nullptr};
    SwrContext* m_swr{nullptr};
    int m_streamIdx{-1};
    int m_channels{2};

    std::thread m_thread;
    std::atomic<bool> m_running{true};
    std::atomic<bool> m_playing{false};
    std::atomic<double> m_position{0.0};
    std::atomic<double> m_duration{0.0};
    std::atomic<float> m_volume{0.5f};
    std::mutex m_trackMutex;
    std::condition_variable m_switchCv;
    std::atomic<bool> m_trackSwitchRequested{false};
    
    std::string m_currentFile;

    std::vector<int16_t> m_decodeBuffer;

    double m_playedSamples = 0.0;


    std::vector<std::string> audioFiles;
    std::unordered_map<std::string, AudioMetadata> metadataCache;
    std::atomic<int> m_currentIndex{-1};

    void playTrackAtIndex(int index);

    std::atomic<bool> m_repeatOne{ false };
    std::atomic<bool> m_shuffle{ false };

    std::vector<int> m_shuffleQueue;
    std::atomic<size_t> m_queuePos{ 0 };
};