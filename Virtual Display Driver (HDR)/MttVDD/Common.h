#pragma once

#include <string>
#include <chrono>
#include <windows.h>

// Forward declaration for the logging function used throughout the driver
void vddlog(const char* type, const char* message);

// Forward declarations for existing string conversion functions
std::string WStringToString(const std::wstring& wstr);
std::wstring StringToWString(const std::string& str);