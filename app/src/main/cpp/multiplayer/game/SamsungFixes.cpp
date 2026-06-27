#include "SamsungFixes.h"

#include <atomic>
#include <dlfcn.h>
#include <cstdint>

#include "../main.h"
#include "../vendor/armhook/patch.h"

namespace {
constexpr uintptr_t kMinValidPtr = 0x1000;
constexpr int kTexturePoolSizeLow = 0x400000;  // 4 MB
constexpr int kTexturePoolSizeHigh = 0x800000; // 8 MB

bool IsLowSpecDeviceSafe()
{
    if (!CHook::lib) {
        return false;
    }

    void* sym = dlsym(CHook::lib, "_Z15IsLowSpecDevicev");
    if (!sym) {
        return false;
    }

    using Func = bool (*)();
    return reinterpret_cast<Func>(sym)();
}
} // namespace

namespace SamsungFixes {
void ApplyTexturePoolLimit()
{
    static std::atomic_bool applied{false};
    if (applied.exchange(true)) {
        return;
    }

    if (!g_libGTASA) {
        return;
    }

    const bool is_low_spec = IsLowSpecDeviceSafe();
    const int texture_pool_size = is_low_spec ? kTexturePoolSizeLow : kTexturePoolSizeHigh;

#if VER_x32
    constexpr uintptr_t kTexturePoolBase = 0x6BCC00;
    constexpr uintptr_t kTexturePoolSizeOffset = 0x10;
#else
    constexpr uintptr_t kTexturePoolBase = 0x84B270;
    constexpr uintptr_t kTexturePoolSizeOffset = 0x18;
#endif

    auto* size_limit_ptr = reinterpret_cast<int*>(
        g_libGTASA + kTexturePoolBase + kTexturePoolSizeOffset);

    if (reinterpret_cast<uintptr_t>(size_limit_ptr) < kMinValidPtr) {
        return;
    }

    *size_limit_ptr = texture_pool_size;
    FLog("SamsungFixes: texture pool limit set to %d (lowSpec=%d)",
         texture_pool_size,
         is_low_spec ? 1 : 0);
}
} // namespace SamsungFixes
