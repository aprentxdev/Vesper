#include "AudioEngine.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <thread>
#include <cstring>
#include <random>
#include <chrono>

AudioEngine::AudioEngine() {
    // Initialize FFmpeg logging and OpenAL device/context
    av_log_set_level(AV_LOG_ERROR);
    m_device = alcOpenDevice(nullptr);
    if (!m_device) throw std::runtime_error("OpenAL: Failed to open device");

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context || !alcMakeContextCurrent(m_context)) {
        alcCloseDevice(m_device);
        throw std::runtime_error("OpenAL: Failed to create context");
    }

    // Generate source and multiple buffers for streaming
    alGenSources(1, &m_source);
    alGenBuffers(NUM_BUFFERS, m_buffers);

    m_decodeBuffer.resize(BUFFER_SAMPLES * 2); // stereo buffer

    // Start worker thread for streaming audio
    m_thread = std::thread(&AudioEngine::workerThread, this);
}

AudioEngine::~AudioEngine() {
    // Stop worker thread
    m_running = false;
    m_switchCv.notify_one();
    if (m_thread.joinable()) m_thread.join();

    // Cleanup
    alDeleteSources(1, &m_source);
    alDeleteBuffers(NUM_BUFFERS, m_buffers);

    if (m_swr) swr_free(&m_swr);
    if (m_codec) avcodec_free_context(&m_codec);
    if (m_fmt) avformat_close_input(&m_fmt);

    alcMakeContextCurrent(nullptr);
    if (m_context) alcDestroyContext(m_context);
    if (m_device) alcCloseDevice(m_device);
}

ALenum AudioEngine::formatFromChannels(int channels) {
    // Return OpenAL format based on channel count
    return channels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
}

bool AudioEngine::openFile(const std::string& path) {
    // Close previous file if open
    if (m_fmt) {
        avformat_close_input(&m_fmt);
        m_fmt = nullptr;
    }

    // Open audio file
    if (avformat_open_input(&m_fmt, path.c_str(), nullptr, nullptr) < 0) return false;
    if (avformat_find_stream_info(m_fmt, nullptr) < 0) return false;

    // Find best audio stream
    m_streamIdx = av_find_best_stream(m_fmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (m_streamIdx < 0) return false;

    // Setup codec context
    AVStream* audio_stream = m_fmt->streams[m_streamIdx];
    const AVCodec* codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (!codec) return false;

    m_codec = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(m_codec, audio_stream->codecpar);
    if (avcodec_open2(m_codec, codec, nullptr) < 0) return false;
    
    m_channels = m_codec->ch_layout.nb_channels;

    // Setup resampler for stereo output
    if (m_swr) swr_free(&m_swr);
    m_swr = swr_alloc();

    AVChannelLayout out_layout{};
    av_channel_layout_default(&out_layout, 2);

    int ret = swr_alloc_set_opts2(
        &m_swr,
        &out_layout,
        AV_SAMPLE_FMT_S16,
        m_codec->sample_rate,
        &m_codec->ch_layout,
        m_codec->sample_fmt,
        m_codec->sample_rate,
        0, nullptr
    );

    if (ret < 0 || swr_init(m_swr) < 0) {
        std::cerr << "Failed to initialize SwrContext\n";
        return false;
    }

    // Store audio duration in seconds
    m_duration.store((double)audio_stream->duration * av_q2d(audio_stream->time_base));
    return true;
}



int AudioEngine::decodeNextBlock(int16_t* outBuffer, int maxSamples) {
    // Decode next block of samples, resample to stereo
    if (!m_fmt || !m_codec) return 0;

    AVPacket packet{};

    AVFrame* frame = av_frame_alloc();
    int totalSamples = 0;

    while (totalSamples < maxSamples && av_read_frame(m_fmt, &packet) >= 0) {
        if (packet.stream_index == m_streamIdx) {
            if (avcodec_send_packet(m_codec, &packet) == 0) {
                while (avcodec_receive_frame(m_codec, frame) == 0) {
                    int outSamples = swr_get_out_samples(m_swr, frame->nb_samples);
                    if (outSamples + totalSamples > maxSamples) outSamples = maxSamples - totalSamples;

                    uint8_t* outBuf[2] = { (uint8_t*)(outBuffer + totalSamples * 2), nullptr };
                    int converted = swr_convert(m_swr, outBuf, outSamples,
                                                (const uint8_t**)frame->data, frame->nb_samples);

                    totalSamples += converted;
                    if (totalSamples >= maxSamples) break;
                }
            }
        }
        av_packet_unref(&packet);
    }

    av_frame_free(&frame);
    return totalSamples;
}

void AudioEngine::loadAndPlay(const std::string& filePath) {
    auto it = std::find(audioFiles.begin(), audioFiles.end(), filePath);
    if (it != audioFiles.end()) {
        m_currentIndex.store(static_cast<int>(it - audioFiles.begin()));
    } else {
        m_currentIndex.store(-1);  // файл не из плейлиста
    }

    // Stop current track and request switch
    {
        std::lock_guard<std::mutex> lock(m_trackMutex);

        m_trackSwitchRequested = true;

        stop();
        

        if (!openFile(filePath)) {
            std::cerr << "Failed to open audio file: " << filePath << "\n";
            m_trackSwitchRequested = false;
            m_switchCv.notify_one();  // wake worker
            return;
        }

        // Fill initial OpenAL buffers
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            int decoded = decodeNextBlock(m_decodeBuffer.data(), BUFFER_SAMPLES);
            if (decoded > 0) {
                alBufferData(m_buffers[i], formatFromChannels(2), m_decodeBuffer.data(),
                            decoded * 2 * sizeof(int16_t), m_codec->sample_rate);
            }
        }

        alSourceQueueBuffers(m_source, NUM_BUFFERS, m_buffers);
        alSourcef(m_source, AL_GAIN, m_volume.load());
        alSourcePlay(m_source);

        m_playing = true;
        m_currentFile = filePath;

        m_trackSwitchRequested = false;
    }
    m_switchCv.notify_one(); // wake worker
}

// Resume playback
void AudioEngine::play() {
    {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        if (!m_playing) {
            alSourcePlay(m_source);
            m_playing = true;
        }
    }
    m_switchCv.notify_one();
}

// Pause playback
void AudioEngine::pause() {
    {
        std::lock_guard<std::mutex> lock(m_trackMutex);
        if (m_playing) {
            alSourcePause(m_source);
            m_playing = false;
        }
    }
    m_switchCv.notify_one();
}

// Toogle play/pause state
void AudioEngine::playPause() {
    ALint state;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    state == AL_PLAYING ? pause() : play();
}

// Stop playback and clear queued buffers
void AudioEngine::stop() {
    alSourceStop(m_source);
    ALint queued;
    alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
    while (queued-- > 0) {
        ALuint buf;
        alSourceUnqueueBuffers(m_source, 1, &buf);
    }
    m_playing = false;
    m_position.store(0.0);
    m_playedSamples = 0;
}

// Worker thread: stream audio
void AudioEngine::workerThread() {
    while (m_running) {
        {
            std::unique_lock<std::mutex> lock(m_trackMutex);
            m_switchCv.wait(lock, [this] {
                return !m_trackSwitchRequested && (m_playing || !m_running);
            });

            if (!m_running) break;

            if (!m_playing) {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            // Handle processed OpenAL buffers
            ALint processed = 0;
            alGetSourcei(m_source, AL_BUFFERS_PROCESSED, &processed);

            while (processed-- > 0) {
                ALuint buf;
                alSourceUnqueueBuffers(m_source, 1, &buf);

                // Update played samples count
                ALint size = 0;
                alGetBufferi(buf, AL_SIZE, &size);
                int played_samples = size / 4;  // 2 channels * 2 bytes
                m_playedSamples += played_samples;

                int decoded = decodeNextBlock(m_decodeBuffer.data(), BUFFER_SAMPLES);
                if (decoded <= 0) {
                    // track has ended
                    lock.unlock();
                    playNext();
                    lock.lock();
                    continue;
                }

                // Refill buffer and queue again
                alBufferData(buf, AL_FORMAT_STEREO16, m_decodeBuffer.data(),
                            decoded * 4, m_codec->sample_rate);
                alSourceQueueBuffers(m_source, 1, &buf);
            }

            // Ensure source is playing
            ALint state;
            alGetSourcei(m_source, AL_SOURCE_STATE, &state);
            if (state != AL_PLAYING && state != AL_PAUSED) {
                alSourcePlay(m_source);
            }

            // Update playback position
            float offsetSec = 0.0f;
            alGetSourcef(m_source, AL_SEC_OFFSET, &offsetSec);
            double positionSec = m_playedSamples / static_cast<double>(m_codec->sample_rate);
            m_position.store(positionSec + offsetSec);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

// Return current file path
std::string AudioEngine::currentFile() const {
    return m_currentFile;
}

// Fetch metadata of current track
std::optional<AudioMetadata> AudioEngine::currentMetadata() const {
    if (m_currentFile.empty()) return std::nullopt;
    auto map = AddAudioFile(m_currentFile);
    auto it = map.find(m_currentFile);
    if (it != map.end()) return it->second;
    return std::nullopt;
}

void AudioEngine::setVolume(float v) {
    v = std::clamp(v, 0.0f, 2.0f);
    m_volume.store(v);
    alSourcef(m_source, AL_GAIN, v);
}

// Seek to position in seconds
void AudioEngine::seek(double seconds) {
    if (!m_fmt || !m_codec) return;

    if (seconds < 0) seconds = 0;
    if (seconds > m_duration.load()) seconds = m_duration.load();

    {
        std::unique_lock<std::mutex> lock(m_trackMutex);

        m_trackSwitchRequested = true;

        m_playedSamples = seconds * m_codec->sample_rate;

        alSourceStop(m_source);

        // Clear queued buffers
        ALint queued;
        alGetSourcei(m_source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0) {
            ALuint buf;
            alSourceUnqueueBuffers(m_source, 1, &buf);
        }

        // Seek FFmpeg stream
        int64_t ts = static_cast<int64_t>(seconds / av_q2d(m_fmt->streams[m_streamIdx]->time_base));
        if (av_seek_frame(m_fmt, m_streamIdx, ts, AVSEEK_FLAG_BACKWARD) < 0) {
            std::cerr << "Failed to seek audio\n";
            m_trackSwitchRequested = false;
            return;
        }
        avcodec_flush_buffers(m_codec);

        // Refill buffers
        for (int i = 0; i < NUM_BUFFERS; ++i) {
            int decoded = decodeNextBlock(m_decodeBuffer.data(), BUFFER_SAMPLES);
            if (decoded <= 0) break;

            alBufferData(m_buffers[i], AL_FORMAT_STEREO16,
                         m_decodeBuffer.data(), decoded * 4, m_codec->sample_rate);
        }

        alSourceQueueBuffers(m_source, NUM_BUFFERS, m_buffers);
        alSourcef(m_source, AL_GAIN, m_volume.load());
        alSourcePlay(m_source);

        m_position.store(seconds);
        m_playing = true;

        m_trackSwitchRequested = false;
    }

    m_switchCv.notify_one(); // wake worker
}

void AudioEngine::AddFilesFromDirectory(const std::string& directory) {
    auto newMetadata = ::AddAudioFilesFromDirectory(directory); // scan directory
    for (auto& [path, meta] : newMetadata) {
        if (std::find(audioFiles.begin(), audioFiles.end(), path) == audioFiles.end()) {
            audioFiles.push_back(path); // add path
            metadataCache[path] = meta; // add metadata
        }
    }
}
void AudioEngine::AddFile(const std::string& filePath) {
    auto newMetadata = ::AddAudioFile(filePath); // get metadata
    for (auto& [path, meta] : newMetadata) {
        if (std::find(audioFiles.begin(), audioFiles.end(), path) == audioFiles.end()) {
            audioFiles.push_back(path); // add path
            metadataCache[path] = meta; // add metadata
        }
    }
}

const std::vector<std::string>& AudioEngine::GetAudioFiles() const {
    return audioFiles;
}

const std::unordered_map<std::string, AudioMetadata>& AudioEngine::GetMetadataCache() const {
    return metadataCache;
}


void AudioEngine::playTrackAtIndex(int index)
{
    if (index < 0 || index >= static_cast<int>(audioFiles.size()))
        return;

    const std::string& path = audioFiles[index];
    loadAndPlay(path);
}

void AudioEngine::playNext()
{
    if (audioFiles.empty()) return;

    int nextIndex = -1;

    if (m_repeatOne.load()) {
        nextIndex = m_currentIndex.load();
        if (nextIndex < 0) nextIndex = 0;
    }
    if (m_shuffle.load() && !m_shuffleQueue.empty()) {
        size_t pos = m_queuePos.load();
        if (pos + 1 < m_shuffleQueue.size()) {
            nextIndex = m_shuffleQueue[pos + 1];
            m_queuePos.store(pos + 1);
        }
        else {
            stop();
            m_currentIndex.store(-1);
            return;
        }
    }
    else {
        int current = m_currentIndex.load();
        nextIndex = current + 1;
        if (nextIndex >= static_cast<int>(audioFiles.size())) {
            stop();
            m_currentIndex.store(-1);
            return;
        }
    }

    if (m_trackSwitchRequested.load()) return;
    playTrackAtIndex(nextIndex);
}

void AudioEngine::playPrev()
{
    if (audioFiles.empty()) return;

    int prevIndex = -1;

    if (m_repeatOne.load()) {
        prevIndex = m_currentIndex.load();
        if (prevIndex < 0) prevIndex = 0;
    }
    if (m_shuffle.load() && !m_shuffleQueue.empty()) {
        size_t pos = m_queuePos.load();
        if (pos > 0) {
            prevIndex = m_shuffleQueue[pos - 1];
            m_queuePos.store(pos - 1);
        }
        else {
            prevIndex = m_shuffleQueue[0];
        }
    }
    else {
        int current = m_currentIndex.load();
        prevIndex = (current <= 0) ? 0 : current - 1;
    }

    if (m_trackSwitchRequested.load()) return;
    playTrackAtIndex(prevIndex);
}

void AudioEngine::setShuffle(bool enabled)
{
    if (m_shuffle.load() == enabled) return;

    m_shuffle.store(enabled);

    if (enabled) {
        m_shuffleQueue.resize(audioFiles.size());
        for (size_t i = 0; i < audioFiles.size(); ++i) {
            m_shuffleQueue[i] = static_cast<int>(i);
        }

        // Fisher-Yates shuffle
        auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::mt19937 rng(static_cast<unsigned>(seed));
        std::shuffle(m_shuffleQueue.begin(), m_shuffleQueue.end(), rng);

        int current = m_currentIndex.load();
        auto it = std::find(m_shuffleQueue.begin(), m_shuffleQueue.end(), current);
        if (it != m_shuffleQueue.end()) {
            m_queuePos.store(std::distance(m_shuffleQueue.begin(), it));
        }
        else {
            m_queuePos.store(0);
        }
    }
    else {
        m_shuffleQueue.clear();
        m_queuePos.store(0);
    }
}