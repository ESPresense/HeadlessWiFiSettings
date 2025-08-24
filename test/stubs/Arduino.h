#pragma once
#include <string>
#include <cstdint>

class String {
    std::string data;
public:
    String() {}
    String(const char* s): data(s ? s : "") {}
    String(char c): data(1, c) {}
    size_t length() const { return data.length(); }
    bool isEmpty() const { return data.empty(); }
    char operator[](size_t i) const { return data[i]; }
    String& operator+=(const char* s) { data += s; return *this; }
    String& operator+=(char c) { data.push_back(c); return *this; }
    String& operator+=(const String& other) { data += other.data; return *this; }
    const char* c_str() const { return data.c_str(); }
};

