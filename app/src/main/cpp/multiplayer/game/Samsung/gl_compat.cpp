#include "gl_compat.h"

#include <cctype>
#include <cstring>
#include <dlfcn.h>
#include <sys/system_properties.h>

namespace {
static GlAlphaFuncFn g_glAlphaFunc = nullptr;
static GlAlphaFuncFn g_glAlphaFuncQcom = nullptr;
static bool g_alphaFuncResolved = false;
static bool g_checked_angle = false;
static bool g_is_angle = false;
static bool g_force_alpha_emulation = false;
static bool g_checked_device = false;

static bool contains_nocase(const char* haystack, const char* needle)
{
    if (!haystack || !needle || !*needle) {
        return false;
    }
    for (const char* h = haystack; *h; ++h) {
        const char* h_it = h;
        const char* n_it = needle;
        while (*h_it && *n_it) {
            unsigned char hc = static_cast<unsigned char>(*h_it);
            unsigned char nc = static_cast<unsigned char>(*n_it);
            if (std::tolower(hc) != std::tolower(nc)) {
                break;
            }
            ++h_it;
            ++n_it;
        }
        if (!*n_it) {
            return true;
        }
    }
    return false;
}

static bool is_samsung_gles_string(const char* str)
{
    return contains_nocase(str, "samsung") || contains_nocase(str, "xclipse");
}

static bool is_samsung_gles_symbol(const void* fn)
{
    if (!fn) {
        return false;
    }
    Dl_info info;
    if (dladdr(fn, &info) && info.dli_fname) {
        return contains_nocase(info.dli_fname, "libGLESv2_samsung");
    }
    return false;
}

static void detect_device_once()
{
    if (g_checked_device) {
        return;
    }
    g_checked_device = true;

    char prop[PROP_VALUE_MAX] = {};

    if (__system_property_get("ro.product.manufacturer", prop) > 0 &&
        contains_nocase(prop, "samsung")) {
        g_force_alpha_emulation = true;
        return;
    }
    if (__system_property_get("ro.product.brand", prop) > 0 &&
        contains_nocase(prop, "samsung")) {
        g_force_alpha_emulation = true;
        return;
    }
    if (__system_property_get("ro.product.vendor.manufacturer", prop) > 0 &&
        contains_nocase(prop, "samsung")) {
        g_force_alpha_emulation = true;
        return;
    }
    if (__system_property_get("ro.hardware", prop) > 0 &&
        (contains_nocase(prop, "samsung") || contains_nocase(prop, "exynos"))) {
        g_force_alpha_emulation = true;
        return;
    }
    if (__system_property_get("ro.board.platform", prop) > 0 &&
        contains_nocase(prop, "exynos")) {
        g_force_alpha_emulation = true;
        return;
    }
}

static void update_context_strings(const char* gl_vendor,
                                   const char* gl_renderer,
                                   const char* gl_version,
                                   const char* egl_vendor,
                                   const char* egl_version)
{
    detect_device_once();

    if (!gl_vendor && !gl_renderer && !gl_version && !egl_vendor && !egl_version) {
        return;
    }

    if ((gl_renderer && strstr(gl_renderer, "ANGLE")) ||
        (gl_vendor && strstr(gl_vendor, "ANGLE")) ||
        (gl_version && strstr(gl_version, "ANGLE")) ||
        (egl_vendor && strstr(egl_vendor, "ANGLE")) ||
        (egl_version && strstr(egl_version, "ANGLE")) ||
        (gl_renderer && strstr(gl_renderer, "Google")) ||
        (gl_vendor && strstr(gl_vendor, "Google"))) {
        g_is_angle = true;
    }

    if (is_samsung_gles_string(gl_renderer) ||
        is_samsung_gles_string(gl_vendor) ||
        is_samsung_gles_string(egl_vendor) ||
        is_samsung_gles_string(egl_version)) {
        g_force_alpha_emulation = true;
    }

    g_checked_angle = true;

    if (g_force_alpha_emulation && g_alphaFuncResolved) {
        g_glAlphaFuncQcom = nullptr;
        g_glAlphaFunc = nullptr;
    }
}
} // namespace

namespace GlCompat {
void UpdateContextStrings(const char* gl_vendor,
                          const char* gl_renderer,
                          const char* gl_version,
                          const char* egl_vendor,
                          const char* egl_version)
{
    update_context_strings(gl_vendor, gl_renderer, gl_version, egl_vendor, egl_version);
}

void DetectAngleOnce()
{
    detect_device_once();
    if (g_checked_angle) {
        return;
    }

    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        return;
    }

    const char* gl_vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* gl_renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    const char* gl_version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* egl_vendor = nullptr;
    const char* egl_version = nullptr;
    EGLDisplay egl_display = eglGetCurrentDisplay();
    if (egl_display != EGL_NO_DISPLAY) {
        egl_vendor = eglQueryString(egl_display, EGL_VENDOR);
        egl_version = eglQueryString(egl_display, EGL_VERSION);
    }

    update_context_strings(gl_vendor, gl_renderer, gl_version, egl_vendor, egl_version);
}

void ResolveAlphaFuncOnce()
{
    detect_device_once();
    if (g_alphaFuncResolved) {
        return;
    }

    if (g_force_alpha_emulation) {
        g_alphaFuncResolved = true;
        return;
    }

    if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
        return;
    }

    g_glAlphaFuncQcom = reinterpret_cast<GlAlphaFuncFn>(eglGetProcAddress("glAlphaFuncQCOM"));
    g_glAlphaFunc = reinterpret_cast<GlAlphaFuncFn>(eglGetProcAddress("glAlphaFunc"));
    if (!g_force_alpha_emulation) {
        if (is_samsung_gles_symbol(reinterpret_cast<void*>(g_glAlphaFuncQcom)) ||
            is_samsung_gles_symbol(reinterpret_cast<void*>(g_glAlphaFunc))) {
            g_force_alpha_emulation = true;
        }
    }
    if (g_force_alpha_emulation) {
        g_glAlphaFuncQcom = nullptr;
        g_glAlphaFunc = nullptr;
    }
    g_alphaFuncResolved = true;
}

bool IsAngle()
{
    return g_is_angle;
}

bool ForceAlphaEmulation()
{
    return g_force_alpha_emulation;
}

bool CanUseAlphaFunc()
{
    return !g_force_alpha_emulation && (g_glAlphaFuncQcom || g_glAlphaFunc);
}

GlAlphaFuncFn GetAlphaFunc()
{
    return g_glAlphaFunc;
}

GlAlphaFuncFn GetAlphaFuncQcom()
{
    return g_glAlphaFuncQcom;
}
} // namespace GlCompat
