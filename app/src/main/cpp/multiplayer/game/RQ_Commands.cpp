//
// Created by x1y2z on 25.05.2023.
//

#include "../game/common.h"

#include <GLES3/gl32.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>
#include <cstring>
#include <cstdlib>

#include "RQ_Commands.h"
#include "game/Samsung/gl_compat.h"
#include "SamsungFixes.h"
#include "../vendor/armhook/patch.h"

bool RQCaps[16];

namespace {
    void (*RQ_Command_rqInit_orig)(uint8 **qData);
    char* DuplicateString(const char* src)
    {
        if (!src) {
            return nullptr;
        }
        const size_t len = std::strlen(src) + 1;
        auto* out = static_cast<char*>(std::malloc(len));
        if (!out) {
            return nullptr;
        }
        std::memcpy(out, src, len);
        return out;
    }
    void RQ_Command_rqInit_reimpl(uint8 **qData)
    {
        (void)qData;

#if VER_x32
        if (RQ_Command_rqInit_orig) {
        RQ_Command_rqInit_orig(qData);
    }
#else
        const bool samsung_safe = GlCompat::ForceAlphaEmulation();
        if (!g_libGTASA) {
            return;
        }

        const char* gl_vendor = nullptr;
        const char* gl_renderer = nullptr;
        const char* gl_ext = nullptr;

        if (!samsung_safe) {
            if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
                return;
            }
            glGetError();
            glFrontFace(GL_CCW);
            gl_vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
            gl_renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
            gl_ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        } else {
            gl_vendor = "Samsung";
            gl_renderer = "Samsung GLES";
        }

        if (!gl_vendor) {
            gl_vendor = "unknown";
        }
        if (!gl_renderer) {
            gl_renderer = "unknown";
        }

        constexpr uintptr_t kRQCapsOffset = 0x896130;
        constexpr uintptr_t kRQMaxBonesOffset = 0x896140;
        constexpr uintptr_t kRQVendorOffset = 0x896148;
        constexpr uintptr_t kRQRendererOffset = 0x896150;

        auto* caps = reinterpret_cast<uint8_t*>(g_libGTASA + kRQCapsOffset);
        auto* max_bones = reinterpret_cast<int*>(g_libGTASA + kRQMaxBonesOffset);
        auto* rq_vendor = reinterpret_cast<char**>(g_libGTASA + kRQVendorOffset);
        auto* rq_renderer = reinterpret_cast<char**>(g_libGTASA + kRQRendererOffset);

        if (caps) {
            std::memset(caps, 0, 16);
        }

        static char* vendor_dup = nullptr;
        static char* renderer_dup = nullptr;
        if (rq_vendor) {
            if (vendor_dup) {
                std::free(vendor_dup);
            }
            vendor_dup = DuplicateString(gl_vendor);
            *rq_vendor = vendor_dup ? vendor_dup : const_cast<char*>(gl_vendor);
        }
        if (rq_renderer) {
            if (renderer_dup) {
                std::free(renderer_dup);
            }
            renderer_dup = DuplicateString(gl_renderer);
            *rq_renderer = renderer_dup ? renderer_dup : const_cast<char*>(gl_renderer);
        }

        if (!samsung_safe && caps && gl_ext) {
            if (std::strcmp(gl_renderer, "Adreno (TM) 320") == 0) {
                caps[0xC] = 1;
            }
            if (std::strstr(gl_ext, "GL_OES_depth24")) {
                caps[0] = 1;
            }
            if (std::strstr(gl_ext, "GL_OES_packed_depth_stencil")) {
                caps[1] = 1;
            }
            if (std::strstr(gl_ext, "GL_NV_depth_nonlinear")) {
                caps[2] = 1;
            }
            if (std::strstr(gl_ext, "GL_EXT_texture_compression_dxt1") ||
                std::strstr(gl_ext, "GL_EXT_texture_compression_s3tc")) {
                caps[3] = 1;
            }
            if (std::strstr(gl_ext, "GL_AMD_compressed_ATC_texture")) {
                caps[0xC] = 1;
                caps[4] = 1;
            }
            if (std::strstr(gl_ext, "GL_IMG_texture_compression_pvrtc")) {
                caps[5] = 1;
            }
            if (!(caps[5] || caps[4] || caps[3])) {
                caps[0xD] = 1;
            }
            if (std::strstr(gl_ext, "GL_OES_rgb8_rgba8")) {
                caps[6] = 1;
            }
            if (std::strstr(gl_ext, "GL_EXT_texture_filter_anisotropic")) {
                caps[7] = 1;
            }
            if (std::strstr(gl_ext, "GL_QCOM_alpha_test")) {
                caps[0xB] = 1;
            }
            if (std::strstr(gl_ext, "GL_QCOM_binning_control")) {
                glHint(0x8FB0u, 0x8FB1u);
                caps[0xA] = 1;
            }
        }

        if (max_bones) {
            if (!samsung_safe) {
                GLint params = 0x80;
                glGetIntegerv(0x8DFBu, &params);
                int bone_limit = 0x80;
                int calc_limit = (params - 0x20) / 3;
                if (calc_limit < bone_limit) {
                    bone_limit = calc_limit;
                }
                *max_bones = bone_limit;
            } else {
                *max_bones = 0x80;
            }
        }

        if (!samsung_safe && caps) {
            if (!caps[0] || caps[0xD] || caps[5]) {
                glPolygonOffset(-10.0f, -1.0f);
            } else {
                glPolygonOffset(-0.15625f, -0.015625f);
            }
        }

        if (eglGetCurrentContext() != EGL_NO_CONTEXT) {
            CHook::CallFunction<void>("_Z15ES2InitTexturesv");
        }
#endif
    }
    void RQ_Command_rqInit_hook(uint8 **qData)
    {
        GlCompat::DetectAngleOnce();
        const bool is_samsung = GlCompat::ForceAlphaEmulation();
        if (!is_samsung && RQ_Command_rqInit_orig) {
            RQ_Command_rqInit_orig(qData);
        } else {
            RQ_Command_rqInit_reimpl(qData);
        }
        SamsungFixes::ApplyTexturePoolLimit();
    }

    void (*RQ_Command_rqSwapBuffers_orig)(uint8 **qData);
    void RQ_Command_rqSwapBuffers_hook(uint8 **qData)
    {
        GlCompat::DetectAngleOnce();
        const bool is_samsung = GlCompat::ForceAlphaEmulation();
        if (!is_samsung && RQ_Command_rqSwapBuffers_orig) {
            RQ_Command_rqSwapBuffers_orig(qData);
            return;
        }

        if (eglGetCurrentContext() == EGL_NO_CONTEXT) {
            return;
        }
        EGLDisplay display = eglGetCurrentDisplay();
        EGLSurface surface = eglGetCurrentSurface(EGL_DRAW);
        if (display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE) {
            eglSwapBuffers(display, surface);
        }
    }
} // namespace

//void RQ_Command_rqInit(uint8 **qData)
//{
//    const uint8 *v1; // r0
//    const uint8 *v2; // r4
//    const char *v3; // r5
//    bool v4; // r0
//    int v5; // r5
//    unsigned int v6; // r0
//    unsigned int v7; // r1
//    GLint params[3]; // [sp+4h] [bp-Ch] BYREF
//
//    glGetError();
//    glFrontFace(0x901u);
//    v1 = glGetString(0x1F00u);
//    rqVendor = j_strdup(v1);
//    v2 = glGetString(0x1F01u);
//    rqRenderer = j_strdup(v2);
//    v3 = (const char *)glGetString(0x1F03u);
//    if ( !strcmp((const char *)v2, "Adreno (TM) 320") )
//    RQCaps[0xC] = 1;
//    if ( strstr(v3, "GL_OES_depth24") )
//    RQCaps[0] = 1;
//    if ( strstr(v3, "GL_OES_packed_depth_stencil") )
//    RQCaps[1] = 1;
//    if ( strstr(v3, "GL_NV_depth_nonlinear") )
//    RQCaps[2] = 1;
//    if ( strstr(v3, "GL_EXT_texture_compression_dxt1") || strstr(v3, "GL_EXT_texture_compression_s3tc") )
//    RQCaps[3] = 1;
//    if ( strstr(v3, "GL_AMD_compressed_ATC_texture") )
//    {
//    RQCaps[0xC] = 1;
//    RQCaps[4] = 1;
//    }
//    if ( strstr(v3, "GL_IMG_texture_compression_pvrtc") )
//    {
//    v4 = 1;
//    RQCaps[5] = 1;
//    }
//    else
//    {
//    v4 = RQCaps[5];
//    }
//    if ( !((v4 || RQCaps[4] || RQCaps[3]) << 0x18) )
//    RQCaps[0xD] = 1;
//    if ( strstr(v3, "GL_OES_rgb8_rgba8") )
//    RQCaps[6] = 1;
//    if ( strstr(v3, "GL_EXT_texture_filter_anisotropic") )
//    RQCaps[7] = 1;
//    if ( strstr(v3, "GL_QCOM_alpha_test") )
//    RQCaps[0xB] = 1;
//    if ( strstr(v3, "GL_QCOM_binning_control") )
//    {
//    glHint(0x8FB0u, 0x8FB1u);
//    RQCaps[0xA] = 1;
//    }
//    if ( (unsigned int)OS_SystemChip() <= OSDC_A4 )
//    {
//    RQCaps[0xE] = 1;
//    RQCaps[8] = 1;
//    }
//    if ( RQCaps[4] && strstr((const char *)v2, (const char *)&dword_1CC4B4) )
//    RQCaps[0xE] = 1;
//    if ( RQCaps[5] && strstr((const char *)v2, (const char *)&dword_1CC4C0) )
//    RQCaps[0xE] = 1;
//    v5 = 0x80;
//    params[0] = 0x80;
//    RQCaps[9] = 0;
//    glGetIntegerv(0x8DFBu, params);
//    if ( (params[0] - 0x20) / 3 < 0x80 )
//    v5 = (params[0] - 0x20) / 3;
//    RQMaxBones = v5;
//    if ( !RQCaps[0] || (RQCaps[0xD] || RQCaps[5]) << 0x18 )
//    {
//    v6 = 0xC1200000;
//    v7 = 0xBF800000;
//    }
//    else
//    {
//    v6 = 0xBE200000;
//    v7 = 0xBC800000;
//    }
//    glPolygonOffset(*(GLfloat *)&v6, *(GLfloat *)&v7);
//    ES2InitTextures();
//}

void CRQ_Commands::rqVertexBufferSelect(uintptr_t **qData)
{
    if (!qData || !*qData)
    {
        glBindBuffer(0x8892, 0);
        return;
    }

    unsigned int buffer = *(*qData)++;

    if (!buffer)
    {
        glBindBuffer(0x8892, 0);
        return;
    }

    glBindBuffer(0x8892, buffer + 8);

    ES2VertexBuffer::curCPUBuffer = nullptr;
}

void CRQ_Commands::InjectHooks() {
    // bool (&RQCaps)[16] = *(bool(*)[16])0xA9B0C8;

    CHook::InlineHook("_Z17RQ_Command_rqInitRPc", &RQ_Command_rqInit_hook, &RQ_Command_rqInit_orig);
    CHook::InlineHook("_Z24RQ_Command_rqSwapBuffersRPc", &RQ_Command_rqSwapBuffers_hook,
                      &RQ_Command_rqSwapBuffers_orig);
    CHook::Redirect("_Z31RQ_Command_rqVertexBufferSelectRPc", &CRQ_Commands::rqVertexBufferSelect);
}
