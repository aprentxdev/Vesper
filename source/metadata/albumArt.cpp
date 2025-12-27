// TO-DO: cache textures

#include "albumArt.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
}


GLuint LoadTextureFromMemory(const unsigned char* data, size_t size) {
    int width, height, channels;

    // Decode image from memory, force 4 channels (RGBA)
    unsigned char* image_data = stbi_load_from_memory(data, (int)size, &width, &height, &channels, 4);
    if (!image_data) {
        std::cerr << "Failed to load image from memory: " << stbi_failure_reason() << std::endl;
        return 0;
    }

    // Generate OpenGL texture
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Upload pixel data to GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Set texture filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Set texture wrapping mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(image_data); // free CPU-side memory
    return textureID;
}

GLuint LoadAlbumArtTexture(const std::string& filename) {
    AVFormatContext* fmt_ctx = nullptr;

    // Open audio file with FFmpeg
    if (avformat_open_input(&fmt_ctx, filename.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open file: " << filename << std::endl;
        return 0;
    }

    // Read stream info
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream info: " << filename << std::endl;
        avformat_close_input(&fmt_ctx);
        return 0;
    }

    // Search for attached pictures (album art)
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket* attached_pic = &stream->attached_pic;
            if (attached_pic->data && attached_pic->size > 0) {
                // Load OpenGL texture from raw image data
                GLuint tex = LoadTextureFromMemory(attached_pic->data, attached_pic->size);
                avformat_close_input(&fmt_ctx);
                return tex; // return tex if found
            }
        }
    }

    // if no album art found
    avformat_close_input(&fmt_ctx);
    return 0;
}