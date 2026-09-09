#define FMT_UNICODE 0 // aviod error: 'Unicode support requires compiling with /utf-8'
#include <spdlog/spdlog.h>

#include "gl_helpers.h"

void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
		SPDLOG_ERROR("[OpenGL Error] ({0}): {1} in {2} line: {3}", error, function, file, line);
        return false;
    }
    return true;
}