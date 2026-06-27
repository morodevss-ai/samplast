#ifndef GL_COMPAT_H
#define GL_COMPAT_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>

typedef void (*GlAlphaFuncFn)(GLenum func, GLfloat ref);

namespace GlCompat {
    void DetectAngleOnce();
    void ResolveAlphaFuncOnce();
    void UpdateContextStrings(const char* gl_vendor,
                              const char* gl_renderer,
                              const char* gl_version,
                              const char* egl_vendor,
                              const char* egl_version);

    bool IsAngle();
    bool ForceAlphaEmulation();
    bool CanUseAlphaFunc();
    GlAlphaFuncFn GetAlphaFunc();
    GlAlphaFuncFn GetAlphaFuncQcom();
}

#endif // GL_COMPAT_H
