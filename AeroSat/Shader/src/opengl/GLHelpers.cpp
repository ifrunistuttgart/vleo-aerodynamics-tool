#include "GLHelpers.h"
#include <iostream>

void GLClearError()
{
    while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
    while (GLenum error = glGetError())
    {
        std::cout << "[OPenGL Error] (" << error << "): " << function << " in " << file << " line: " << line << std::endl;
        return false;
    }
    return true;
}