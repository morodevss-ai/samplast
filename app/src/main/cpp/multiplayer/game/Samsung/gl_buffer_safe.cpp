#include "gl_buffer_safe.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cstdint>

#include "main.h"

namespace {
constexpr uintptr_t kMinValidPtr = 0x1000;

static bool is_probably_valid_ptr(uintptr_t ptr)
{
    return ptr >= kMinValidPtr;
}
} // namespace

namespace GlBufferSafe {
void BindArrayBufferFromRw(uint32_t buffer_ptr)
{
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        return;
    }

    if (!is_probably_valid_ptr(buffer_ptr)) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return;
    }

    uint32_t vbo = *reinterpret_cast<uint32_t*>(buffer_ptr + 8);
    if (vbo == 0) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
#if VER_x32
    *reinterpret_cast<uint32_t*>(g_libGTASA + 0x6B8AF0) = 0;
#endif
}

void DeleteArrayBufferFromRw(uint32_t* buffer)
{
    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        return;
    }

    if (!is_probably_valid_ptr(reinterpret_cast<uintptr_t>(buffer))) {
        return;
    }

    GLuint id = buffer[2];
    if (id != 0) {
        glDeleteBuffers(1, &id);
    }

    buffer[2] = 0;

#if !VER_x32
    uintptr_t vtbl = static_cast<uintptr_t>(*buffer);
    if (is_probably_valid_ptr(vtbl)) {
        auto dtor = reinterpret_cast<void (*)(uint32_t*)>(vtbl + 4);
        if (dtor) {
            dtor(buffer);
        }
    }
#endif
}
} // namespace GlBufferSafe
