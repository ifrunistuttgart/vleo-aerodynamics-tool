#pragma once

#include <GL/glew.h>

#if defined(_MSC_VER)
    // Microsoft Visual C++
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__GNUC__) || defined(__clang__)
    #if defined(_WIN32) || defined(_WIN64)
        // GCC or Clang on Windows (MinGW / Strawberry Perl C++)
        #define DEBUG_BREAK() __builtin_trap()
    #else
        // GCC or Clang on Linux / macOS
        #include <csignal>
        #define DEBUG_BREAK() raise(SIGTRAP)
    #endif
#else
    #include <csignal>
    #define DEBUG_BREAK() raise(SIGABRT)
#endif

#define ASSERT(x) if (!(x)) DEBUG_BREAK();
#define GLCall(x) GLClearError();\
    x;\
    ASSERT(GLLogCall(#x, __FILE__,__LINE__))

void GLClearError();

bool GLLogCall(const char* function, const char* file, int line);
