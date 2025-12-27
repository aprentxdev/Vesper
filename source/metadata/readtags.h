#pragma once

#include <iostream>
using std::string;

void ReadAudioTags(const char* filename, string* title, string* artist, string* album, int* year, std::string* date_str = nullptr);