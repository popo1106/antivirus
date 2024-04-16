#pragma once
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <unordered_set>
#include <Windows.h>
#include <io.h>       // Include for _setmode

class ScopedSetMode {
public:
    ScopedSetMode(int mode) : originalMode(0) {
        originalMode = _setmode(_fileno(stdout), mode);
    }

    ~ScopedSetMode() {
        _setmode(_fileno(stdout), originalMode);
    }

private:
    int originalMode;
};