//
// Created by roman on 11/19/2024.
//

#include <dlfcn.h>
#include "CUtil.h"
#include "game/Textures/TextureDatabaseRuntime.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/imgui/stb_image_write.h"
#include "patch.h"

struct RwRaster* GetRWRasterFromBitmapPalette(uint8_t* pBitmap, size_t dwStride, size_t dwX, size_t dwY, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (!pBitmap)
    {
        return nullptr;
    }
    int len;
    unsigned char* png = stbi_write_png_to_mem(pBitmap, dwStride, dwX, dwY, 1, &len);
    if (!png)
    {
        return nullptr;
    }
    /*
     * t:001D6F84 ; _DWORD __fastcall RtPNGImageRead(const char *)
.text:001D6F84                 EXPORT _Z14RtPNGImageReadPKc
.text:001D6F84 _Z14RtPNGImageReadPKc                   ; CODE XREF: TextureDatabaseEntry::LoadPNG(char const*)+A↑p
.text:001D6F84

     * .text:001D6F9A                 MOVS            R0, #2
.text:001D6F9C                 MOVS            R1, #1
.text:001D6F9E                 BL              _Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv ; RwStreamOpen(RwStreamType,RwStreamAccessType,void const*)
     */

#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x0020A404, (uintptr_t)"\x03\x20", 2); // rwSTREAMMEMORY
#else
    CHook::WriteMemory(g_libGTASA + 0x002AD8E8, (uintptr_t)"\x60\x00\x80\x52", 4); // rwSTREAMMEMORY
#endif

    RwMemory memoryImage;
    RwInt32 width, height, depth, flags;

    memoryImage.start = (RwUInt8*)png;
    memoryImage.length = (RwUInt32)len;

    RwImage* pImage = RtPNGImageRead((const RwChar*)& memoryImage);

    if (a)
    {
        pImage->palette->alpha = a;
    }
    if (r)
    {
        pImage->palette->red = r;
    }
    if (g)
    {
        pImage->palette->green = g;
    }
    if (b)
    {
        pImage->palette->blue = b;
    }

    if (!pImage)
    {
        STBIW_FREE(png);
        return nullptr;
    }

    RwImageFindRasterFormat(pImage, 4, &width, &height, &depth, &flags);

    RwRaster* pRaster = RwRasterCreate(width, height, depth, flags);

    if (!pRaster)
    {
        STBIW_FREE(png);
        RwImageDestroy(pImage);
        return nullptr;
    }

    RwRasterSetFromImage(pRaster, pImage);

    RwImageDestroy(pImage);
    STBIW_FREE(png);

    /*
     * t:001D6F84 ; _DWORD __fastcall RtPNGImageRead(const char *)
.text:001D6F84                 EXPORT _Z14RtPNGImageReadPKc
.text:001D6F84 _Z14RtPNGImageReadPKc                   ; CODE XREF: TextureDatabaseEntry::LoadPNG(char const*)+A↑p
.text:001D6F84

     * .text:001D6F9A                 MOVS            R0, #2
.text:001D6F9C                 MOVS            R1, #1
.text:001D6F9E                 BL              _Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv ; RwStreamOpen(RwStreamType,RwStreamAccessType,void const*)
     */
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x0020A404, (uintptr_t)"\x02\x20", 2); // rwSTREAMMEMORY
#else
    CHook::WriteMemory(g_libGTASA + 0x002AD8E8, (uintptr_t)"\x40\x00\x80\x52", 4); // rwSTREAMMEMORY
#endif
    return pRaster;
}

struct RwRaster* GetRWRasterFromBitmap(uint8_t* pBitmap, size_t dwStride, size_t dwX, size_t dwY)
{
    if (!pBitmap)
    {
        return nullptr;
    }
    int len;
    unsigned char* png = stbi_write_png_to_mem(pBitmap, dwStride, dwX, dwY, 1, &len);
    if (!png)
    {
        return nullptr;
    }

    /*
     * t:001D6F84 ; _DWORD __fastcall RtPNGImageRead(const char *)
.text:001D6F84                 EXPORT _Z14RtPNGImageReadPKc
.text:001D6F84 _Z14RtPNGImageReadPKc                   ; CODE XREF: TextureDatabaseEntry::LoadPNG(char const*)+A↑p
.text:001D6F84

     * .text:001D6F9A                 MOVS            R0, #2
.text:001D6F9C                 MOVS            R1, #1
.text:001D6F9E                 BL              _Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv ; RwStreamOpen(RwStreamType,RwStreamAccessType,void const*)
     */
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x0020A404, (uintptr_t)"\x03\x20", 2); // rwSTREAMMEMORY
#else
    CHook::WriteMemory(g_libGTASA + 0x002AD8E8, (uintptr_t)"\x60\x00\x80\x52", 4); // rwSTREAMMEMORY
#endif

    RwMemory memoryImage;
    RwInt32 width, height, depth, flags;

    memoryImage.start = (RwUInt8*)png;
    memoryImage.length = (RwUInt32)len;

    RwImage* pImage = RtPNGImageRead((const RwChar*)& memoryImage);

    if (!pImage)
    {
        STBIW_FREE(png);
        return nullptr;
    }

    RwImageFindRasterFormat(pImage, 4, &width, &height, &depth, &flags);

    RwRaster* pRaster = RwRasterCreate(width, height, depth, flags);

    if (!pRaster)
    {
        STBIW_FREE(png);
        RwImageDestroy(pImage);
        return nullptr;
    }

    RwRasterSetFromImage(pRaster, pImage);

    RwImageDestroy(pImage);
    STBIW_FREE(png);

    /*
     * t:001D6F84 ; _DWORD __fastcall RtPNGImageRead(const char *)
.text:001D6F84                 EXPORT _Z14RtPNGImageReadPKc
.text:001D6F84 _Z14RtPNGImageReadPKc                   ; CODE XREF: TextureDatabaseEntry::LoadPNG(char const*)+A↑p
.text:001D6F84

     * .text:001D6F9A                 MOVS            R0, #2
.text:001D6F9C                 MOVS            R1, #1
.text:001D6F9E                 BL              _Z12RwStreamOpen12RwStreamType18RwStreamAccessTypePKv ; RwStreamOpen(RwStreamType,RwStreamAccessType,void const*)
     */
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x0020A404, (uintptr_t)"\x02\x20", 2); // rwSTREAMMEMORY
#else
    CHook::WriteMemory(g_libGTASA + 0x002AD8E8, (uintptr_t)"\x40\x00\x80\x52", 4); // rwSTREAMMEMORY
#endif

    return pRaster;
}

namespace {
struct TexDbSpec {
    const char* name;
    TextureDatabaseFormat format;
};
constexpr TexDbSpec kTexDbSpecs[] = {
    {"samp", TextureDatabaseFormat::DF_Default},
    {"mobile", TextureDatabaseFormat::DF_Default},
    {"txd", TextureDatabaseFormat::DF_Default},
    {"gta3", TextureDatabaseFormat::DF_Default},
    {"gta_int", TextureDatabaseFormat::DF_Default},
    {"player", TextureDatabaseFormat::DF_PVR},
    {"menu", TextureDatabaseFormat::DF_PVR},
	//{"cars", TextureDatabaseFormat::DF_PVR},
    //{"skins", TextureDatabaseFormat::DF_PVR}
};
constexpr int kTexDbCount = static_cast<int>(sizeof(kTexDbSpecs) / sizeof(kTexDbSpecs[0]));

TDBArray<TextureDatabaseRuntime*>* GetRegisteredTexDbList() {
    static auto* registered = reinterpret_cast<TDBArray<TextureDatabaseRuntime*>*>(
        CHook::getSym("_ZN22TextureDatabaseRuntime10registeredE")
    );
    return registered;
}

bool IsTexDbRegistered(const TDBArray<TextureDatabaseRuntime*>* registered, const TextureDatabaseRuntime* db) {
    if (!registered || !registered->dataPtr || !registered->numEntries) {
        return false;
    }
    for (unsigned int i = 0; i < registered->numEntries; ++i) {
        if (registered->dataPtr[i] == db) {
            return true;
        }
    }
    return false;
}
} // namespace


uintptr_t CUtil::FindLib(const char* libname)
{
    FLog("FindLib: loading %s", libname);
    void* handle = dlopen(libname, RTLD_LAZY);
    if (handle) {
        void* symbol = dlsym(handle, "JNI_OnLoad");
        if (symbol) {
            Dl_info info;
            if (dladdr(symbol, &info) != 0) {
                return reinterpret_cast<uintptr_t>(info.dli_fbase);
            }
        }
        dlclose(handle);
    }
    else {
        FLog("FindLib: dlopen failed for %s: %s", libname, dlerror());
    }

    return 0;
}

RwTexture* CUtil::LoadTextureFromDB(const char* dbname, const char* texture)
{
    EnsureTextureDatabasesRegistered();
    TextureDatabaseRuntime* db_handle = TextureDatabaseRuntime::GetDatabase(dbname);
    if(!db_handle)
    {
        FLog("Error: Database not found! (%s)", dbname);
        return nullptr;
    }

    auto* registered = GetRegisteredTexDbList();
    const bool wasRegistered = IsTexDbRegistered(registered, db_handle);
    if (!wasRegistered) {
        TextureDatabaseRuntime::Register(db_handle);
    }

    auto tex = CUtil::GetTexture(texture);
    if(!tex)
    {
        FLog("Error: Texture (%s) not found in database (%s)", dbname, texture);
        if (!wasRegistered) {
            TextureDatabaseRuntime::Unregister(db_handle);
        }
        return nullptr;
    }

    if (!wasRegistered) {
        TextureDatabaseRuntime::Unregister(db_handle);
    }

    return tex;
}

void CUtil::EnsureTextureDatabasesRegistered()
{
    static bool s_attemptedLoad[kTexDbCount] = {};
    auto* registered = GetRegisteredTexDbList();

    for (int i = 0; i < kTexDbCount; ++i)
    {
        const TexDbSpec& spec = kTexDbSpecs[i];
        TextureDatabaseRuntime* db = TextureDatabaseRuntime::GetDatabase(spec.name);
        if (!db && !s_attemptedLoad[i])
        {
            db = TextureDatabaseRuntime::Load(spec.name, false, spec.format);
            s_attemptedLoad[i] = true;
        }

        if (!db) {
            continue;
        }

        if (!IsTexDbRegistered(registered, db)) {
            TextureDatabaseRuntime::Register(db);
        }
    }
}

RwTexture* CUtil::GetTexture(const char* name)
{
    EnsureTextureDatabasesRegistered();
    auto tex = TextureDatabaseRuntime::GetTexture(name);
    if (!tex)
    {
        //tex = CUtil::LoadTextureFromDB("gta3", "ahoodfence2");
        FLog("WARNING! No tex = %s", name);
        return nullptr;
    }
    ++tex->refCount;

    return tex;
}

void __fastcall CUtil::TransformPoint(RwV3d &result, const CSimpleTransform &t, const RwV3d &v)
{
    float cos_heading = cosf(t.m_fHeading);
    float sin_heading = sinf(t.m_fHeading);

    result = {
            t.m_vPosn.x + cos_heading * v.x - sin_heading * v.y,
            t.m_vPosn.y + sin_heading * v.x + cos_heading * v.y,
            v.z + t.m_vPosn.z
    };
}