#pragma once

#ifdef EMSCRIPTEN
    #include <GLES3/gl3.h>
#elifdef TARGET_PLATFORM_macos
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/OpenGL.h>
    #include <OpenGL/gl3.h>
#else
    #error please add your own opengl headers here :)
#endif // ifdef EMSCRIPTEN

#ifdef EMSCRIPTEN
#   define GLSL_PREFIX "#version 300 es\n"
#else
#   define GLSL_PREFIX "#version 330\n"
#endif // ifdef EMSCRIPTEN
