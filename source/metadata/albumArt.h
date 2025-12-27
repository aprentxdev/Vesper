#pragma once

#include <GL/glew.h>        
#include <iostream>
#include <string>
#include <GLFW/glfw3.h>
#include <codecvt>
#include <locale>

GLuint LoadTextureFromMemory(const unsigned char* data, size_t size);
GLuint LoadAlbumArtTexture(const std::string& filename);