#pragma once
#include <string>
#include <cstdint>
#include <cstdlib>

#ifndef F
#define F(x) x
#endif

class String {
    std::string data;
public:
    String() {}
    String(const char* s): data(s ? s : "") {}
    String(char c): data(1, c) {}
    String(int value): data(std::to_string(value)) {}
    String(long value): data(std::to_string(value)) {}
    String(float value): data(std::to_string(value)) {}
    size_t length() const { return data.length(); }
    bool isEmpty() const { return data.empty(); }
    char operator[](size_t i) const { return data[i]; }
    String& operator+=(const char* s) { data += s; return *this; }
    String& operator+=(char c) { data.push_back(c); return *this; }
    String& operator+=(const String& other) { data += other.data; return *this; }
    bool operator==(const char* s) const { return data == (s ? s : ""); }
    bool operator==(const String& other) const { return data == other.data; }
    bool operator!=(const char* s) const { return !(*this == s); }
    bool operator!=(const String& other) const { return !(*this == other); }
    String substring(size_t start) const { return start < data.length() ? data.substr(start).c_str() : ""; }
    void replace(const char* from, const String& to) {
        std::string needle = from ? from : "";
        if (needle.empty()) return;
        size_t pos = 0;
        while ((pos = data.find(needle, pos)) != std::string::npos) {
            data.replace(pos, needle.length(), to.data);
            pos += to.data.length();
        }
    }
    long toInt() const { return std::strtol(data.c_str(), nullptr, 10); }
    float toFloat() const { return std::strtof(data.c_str(), nullptr); }
    const char* c_str() const { return data.c_str(); }
};
