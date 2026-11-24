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

/** @brief Breaks into the debugger (or traps) if x is false. Expands to an unbraced if. */
#define ASSERT(x)                                                                                                      \
    if (!(x))                                                                                                          \
        DEBUG_BREAK();
/**
 * @brief Runs the GL call x, logs every GL error it raised and breaks into the debugger on one.
 *
 * Expands to several statements, so wrap it in braces when it is the body of an unbraced if,
 * else or loop.
 */
#define GLCall(x)                                                                                                      \
    GLClearError();                                                                                                    \
    x;                                                                                                                 \
    ASSERT(GLLogCall(#x, __FILE__, __LINE__))

namespace vat::gl {

/** @brief Discards all pending GL errors (glGetError until GL_NO_ERROR). */
void GLClearError();

/**
 * @brief Logs the first pending GL error, if any, with the call that raised it.
 *
 * @param function Text of the GL call.
 * @param file Source file of the call.
 * @param line Source line of the call.
 * @return True if no error was pending.
 */
bool GLLogCall(const char* function, const char* file, int line);

} // namespace vat::gl
