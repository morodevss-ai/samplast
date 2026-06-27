#include <GLES2/gl2.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <unordered_set>
#include "../main.h"
#include "../settings.h"
#include "../vendor/armhook/patch.h"
#include "game/Samsung/gl_compat.h"
#include "game.h"
#include "../net/netgame.h"
#include "../gui/gui.h"
#include "Widgets/TouchInterface.h"
#include "Textures/TextureDatabase.h"
#include "Textures/TextureDatabaseEntry.h"
#include "Textures/TextureDatabaseRuntime.h"
#include "Scene.h"
#include "sprite2d.h"
#include "Entity/PlayerPedGta.h"
#include "Pools.h"
#include "java/jniutil.h"
#include "game/Models/ModelInfo.h"
#include "MatrixLink.h"
#include "MatrixLinkList.h"
#include "game/Collision/Collision.h"
#include "TxdStore.h"
#include "game/Samsung/gl_buffer_safe.h"
#include "util/CUtil.h"
#include "Coronas.h"
#include "multitouch.h"
#include "Streaming.h"
#include "References.h"
#include "VisibilityPlugins.h"
#include "Timer.h"
#include "game/Animation/AnimManager.h"
#include "FileLoader.h"
#include "CrossHair.h"
#include "RW/RenderWare.h"
#include "RW/pipe/p2core.h"
#include "RW/immedi.h"
#include "World.h"
#include "patches.h"
#include "Core/Rect.h"
#include "rgba.h"
#include "Mobile/MobileSettings/MobileSettings.h"
#include "pad.h"
#include "../voice/Record.h"
#include "../voice/Playback.h"
#include "../voice/PluginConfig.h"
#include "../../vendor/armhook//patch.h"
#include "graphics/CSkyBox.h"
#include "CFirstPersonCamera.hpp"
#include "game/Samsung/NavigationPauseFix.h"
#include "Renderer.h"
#include "Draw.h"

extern "C"
{
#include "xrtutils/ResourceCrypt/encryption/aes.h"
}
#include "xrtutils/ResourceCrypt/encryption/common.h"
#include "xrtutils/ResourceCrypt/CGameResourcesDecryptor.h"

#include "xrtutils/str_obfuscator_no_template.hpp"

#define MAX_ENCRYPTED_TXD 3

const cryptor::string_encryptor encrArch[MAX_ENCRYPTED_TXD] = {
        cryptor::create("texdb/txd/txd.txt", 19),
        cryptor::create("texdb/gta3/gta3.txt", 21),
        cryptor::create("texdb/gta_int/gta_int.txt", 27),
};

bool isEncrypted(const char* szArch)
{
    return false;
    for (int64 i = 0; i < MAX_ENCRYPTED_TXD; i++)
    {
        if (!strcmp(encrArch[i].decrypt(), szArch))
            return true;
    }

    return false;
}

void InitCTX(AES_ctx& ctx, const uint8_t* pKey)
{
    uint8_t key[16];
    memcpy(&key[0], &pKey[0], 16);
    for (int i = 0; i < 16; i++)
    {
        key[i] = XOR_UNOBFUSCATE(key[i]);
    }
    uint8_t iv[16];
    memcpy(&iv[0], &g_iIV, 16);
    for (int i = 0; i < 16; i++)
    {
        iv[i] = XOR_UNOBFUSCATE(iv[i]);
    }
    AES_init_ctx_iv(&ctx, &key[0], &iv[0]);
}

int lastOpenedFile;


static void (*CAudioZones__Update_orig)(bool forceUpdate, CVector pos);
static void CAudioZones__Update_hook(bool forceUpdate, CVector pos)
{
#if !VER_x32
    constexpr uintptr_t kLastUpdateCoorsGot = 0x84B288;
    constexpr uintptr_t kLastUpdateCoorsData = 0xBBA4D8; // symbol: LastUpdateCoors

    uintptr_t* slot = reinterpret_cast<uintptr_t*>(g_libGTASA + kLastUpdateCoorsGot);
    uintptr_t target = slot ? *slot : 0;
    uintptr_t expected = g_libGTASA + kLastUpdateCoorsData;
    if (!slot || target != expected) {
        static bool s_logged = false;
        if (!s_logged) {
            FLog("CAudioZones::Update skipped (invalid LastUpdateCoors ptr): got=0x%llx expected=0x%llx",
                 (unsigned long long)target, (unsigned long long)expected);
            s_logged = true;
        }
        return;
    }
#endif

    if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) {
        return;
    }

    CAudioZones__Update_orig(forceUpdate, pos);
}

extern UI* pUI;
extern CGame* pGame;
extern CNetGame *pNetGame;
extern MaterialTextGenerator* pMaterialTextGenerator;
extern CSettings* pSettings;
extern void ShowJavaHudEditorFromMenu();

static void (*CFont_PrintString)(float posX, float posY, uint16_t* text);

static bool g_gameOptionsHidden = false;
static uint32_t g_nextTabRefreshMs = 0;

#if !VER_x32
constexpr uintptr_t kSettingsScreenOnLanguageOptions = 0x35AD1C;
constexpr uintptr_t kSelectScreenOnSocialClub = 0x3634FC;
#endif

static void (*ControlsScreen_DoAdjustableHUD)(void* screen, int index);
static void (*SelectScreen_AddItem_orig)(void* screen, void* item);
static uint16_t* (*MenuSelection_GetTitle)(void* thiz);
static void (*CMenuManager_DrawFrontEnd)(void* thiz);
static void (*CMenuManager_Process)(void* thiz);
static void (*MobileMenu_Update)(void* thiz);
static void (*MobileMenu_Render)(void* thiz);
static bool g_skipMobileMenuRenderOnce = false;
static bool g_inMobileMenuRender = false;
static std::unordered_set<RwRaster*> g_validRasters;

static const char kVoiceChatMenuKey[] = "VCHAT";
static const char kAndroidKeyboardMenuKey[] = "ANDKBD";
static const char kFpsLimitMenuKey[] = "FPSLIM";
static char16_t g_voiceChatMenuTitle[64]{};
static char16_t g_androidKeyboardMenuTitle[64]{};
static char16_t g_fpsLimitMenuTitle[64]{};

static constexpr int kFpsLimitMin = 30;
static constexpr int kFpsLimitMax = 120;
static constexpr int kFpsLimitStep = 10;

extern CJavaWrapper *pJavaWrapper;

struct MenuActionSelection {
    void* vtable;
    const char* key;
    void* action;
    int param;
    uint32_t pad0;
    uint32_t pad1;
};

static bool IsLocalPlayerSpawned()
{
    if (!pNetGame) {
        return false;
    }
    CPlayerPool* pool = pNetGame->GetPlayerPool();
    if (!pool) {
        return false;
    }
    CLocalPlayer* local = pool->GetLocalPlayer();
    return local && local->m_bIsActive;
}

static void AsciiToUtf16(const char* src, char16_t* dst, size_t dstSize)
{
    if (!dstSize) {
        return;
    }
    size_t out = 0;
    if (src) {
        for (; src[out] && out + 1 < dstSize; ++out) {
            dst[out] = static_cast<char16_t>(src[out]);
        }
    }
    dst[out] = u'\0';
}

static int ClampFpsLimit(int fps)
{
    if (fps < kFpsLimitMin) {
        return kFpsLimitMin;
    }
    if (fps > kFpsLimitMax) {
        return kFpsLimitMax;
    }
    return fps;
}

static void UpdateVoiceChatMenuTitle()
{
    const bool enabled = pSettings ? pSettings->Get().bVoiceChatEnable : true;
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "Voice Chat: %s", enabled ? "On" : "Off");
    AsciiToUtf16(buffer, g_voiceChatMenuTitle, sizeof(g_voiceChatMenuTitle) / sizeof(g_voiceChatMenuTitle[0]));
}

static void UpdateAndroidKeyboardMenuTitle()
{
    const bool enabled = pSettings ? pSettings->Get().iAndroidKeyboard : true;
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "Android Keyboard: %s", enabled ? "On" : "Off");
    AsciiToUtf16(buffer, g_androidKeyboardMenuTitle,
                 sizeof(g_androidKeyboardMenuTitle) / sizeof(g_androidKeyboardMenuTitle[0]));
}

static void UpdateFpsLimitMenuTitle()
{
    int fps = pSettings ? ClampFpsLimit(pSettings->Get().iFPSCount) : kFpsLimitMax;
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "FPS Limit: %d", fps);
    AsciiToUtf16(buffer, g_fpsLimitMenuTitle, sizeof(g_fpsLimitMenuTitle) / sizeof(g_fpsLimitMenuTitle[0]));
}

static void UpdateGameOptionsMenuTitles()
{
    UpdateVoiceChatMenuTitle();
    UpdateAndroidKeyboardMenuTitle();
    UpdateFpsLimitMenuTitle();
}

static void ApplyVoiceChatSetting(bool enabled)
{
    PluginConfig::SetMicroEnable(enabled);
    PluginConfig::SetSoundEnable(enabled);
    Record::SetMicroEnable(enabled);
    Playback::SetSoundEnable(enabled);
    if (!enabled) {
        Record::StopRecording();
    }
}

static void ToggleVoiceChatOption(void* screen, int index)
{
    if (!pSettings) {
        return;
    }
    auto& settings = pSettings->Get();
    settings.bVoiceChatEnable = !settings.bVoiceChatEnable;
    ApplyVoiceChatSetting(settings.bVoiceChatEnable);
    UpdateVoiceChatMenuTitle();
}

static void ToggleAndroidKeyboardOption(void* screen, int index)
{
    if (!pSettings) {
        return;
    }
    auto& settings = pSettings->Get();
    settings.iAndroidKeyboard = !settings.iAndroidKeyboard;
    UpdateAndroidKeyboardMenuTitle();
}

static void CycleFpsLimitOption(void* screen, int index)
{
    if (!pSettings) {
        return;
    }
    auto& settings = pSettings->Get();
    int fps = ClampFpsLimit(settings.iFPSCount);
    fps += kFpsLimitStep;
    if (fps > kFpsLimitMax) {
        fps = kFpsLimitMin;
    }
    settings.iFPSCount = fps;
    ApplyFPSPatch(static_cast<uint8_t>(fps));
    UpdateFpsLimitMenuTitle();
}

static void ControlsScreen_DoHudEditor(void* screen, int index)
{
    if (pNetGame && pNetGame->GetGameState() == GAMESTATE_CONNECTED && IsLocalPlayerSpawned()) {
        CHook::CallFunction<void>(g_libGTASA + 0x3682F0); // Menu_SwitchOffToGame
        CTimer::EndUserPause();
        ShowJavaHudEditorFromMenu();
        return;
    }

    if (ControlsScreen_DoAdjustableHUD) {
        ControlsScreen_DoAdjustableHUD(screen, index);
    }
}

static void InitActionSelection(MenuActionSelection* item, const char* key, void* action)
{
    if (!item) {
        return;
    }
    memset(item, 0, sizeof(MenuActionSelection));
    item->vtable = reinterpret_cast<void*>(g_libGTASA + 0x825f28 + 0x10);
    item->key = key;
    item->action = action;
    item->param = 0;
}

static bool MenuHasItemKey(void* screen, const char* key)
{
    if (!screen || !key) {
        return false;
    }
    const int count = *reinterpret_cast<int*>(
            reinterpret_cast<char*>(screen) + 0x24);
    void** items = *reinterpret_cast<void***>(
            reinterpret_cast<char*>(screen) + 0x28);
    if (!items || count <= 0) {
        return false;
    }
    for (int i = 0; i < count; ++i) {
        void* entry = items[i];
        if (!entry) {
            continue;
        }
        const char* entryKey = *reinterpret_cast<const char**>(
                reinterpret_cast<char*>(entry) + 0x8);
        if (entryKey == key) {
            return true;
        }
    }
    return false;
}

static void RemoveMenuItemByAction(void* screen, void* action)
{
    if (!screen || !action) {
        return;
    }
    int count = *reinterpret_cast<int*>(
            reinterpret_cast<char*>(screen) + 0x24);
    void** items = *reinterpret_cast<void***>(
            reinterpret_cast<char*>(screen) + 0x28);
    if (!items || count <= 0) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        void* entry = items[i];
        if (!entry) {
            continue;
        }
        void* entryAction = *reinterpret_cast<void**>(
                reinterpret_cast<char*>(entry) + 0x10);
        if (entryAction != action) {
            continue;
        }
        if (i + 1 < count) {
            memmove(&items[i], &items[i + 1],
                    static_cast<size_t>(count - i - 1) * sizeof(void*));
        }
        --count;
        *reinterpret_cast<int*>(
                reinterpret_cast<char*>(screen) + 0x24) = count;
        --i;
    }
}

static int FindMenuItemIndexByAction(void* screen, void* action)
{
    if (!screen || !action) {
        return -1;
    }
    int count = *reinterpret_cast<int*>(
            reinterpret_cast<char*>(screen) + 0x24);
    void** items = *reinterpret_cast<void***>(
            reinterpret_cast<char*>(screen) + 0x28);
    if (!items || count <= 0) {
        return -1;
    }
    for (int i = 0; i < count; ++i) {
        void* entry = items[i];
        if (!entry) {
            continue;
        }
        void* entryAction = *reinterpret_cast<void**>(
                reinterpret_cast<char*>(entry) + 0x10);
        if (entryAction == action) {
            return i;
        }
    }
    return -1;
}

static void RemoveMenuItemByKey(void* screen, const char* key, void** outItem)
{
    if (!screen || !key || !outItem) {
        return;
    }
    int* countPtr = reinterpret_cast<int*>(
            reinterpret_cast<char*>(screen) + 0x24);
    void** items = *reinterpret_cast<void***>(
            reinterpret_cast<char*>(screen) + 0x28);
    if (!items || !countPtr || *countPtr <= 0) {
        return;
    }
    int count = *countPtr;
    for (int i = 0; i < count; ++i) {
        void* entry = items[i];
        if (!entry) {
            continue;
        }
        const char* entryKey = *reinterpret_cast<const char**>(
                reinterpret_cast<char*>(entry) + 0x8);
        if (entryKey != key) {
            continue;
        }
        *outItem = entry;
        if (i + 1 < count) {
            memmove(&items[i], &items[i + 1],
                    static_cast<size_t>(count - i - 1) * sizeof(void*));
        }
        --count;
        *countPtr = count;
        --i;
    }
}

static void InsertMenuItemAt(void* screen, int index, void* item)
{
    if (!screen || !item || index < 0) {
        return;
    }
    int* countPtr = reinterpret_cast<int*>(
            reinterpret_cast<char*>(screen) + 0x24);
    void** items = *reinterpret_cast<void***>(
            reinterpret_cast<char*>(screen) + 0x28);
    if (!items || !countPtr) {
        return;
    }
    int count = *countPtr;
    if (index > count) {
        index = count;
    }
    if (count - index > 0) {
        memmove(&items[index + 1], &items[index],
                static_cast<size_t>(count - index) * sizeof(void*));
    }
    items[index] = item;
    *countPtr = count + 1;
}

static void ReorderGameOptionsItems(void* screen, void* restoreAction)
{
    if (!screen || !restoreAction) {
        return;
    }
    void* fpsItem = nullptr;
    void* voiceItem = nullptr;
    void* keyboardItem = nullptr;
    RemoveMenuItemByKey(screen, kFpsLimitMenuKey, &fpsItem);
    RemoveMenuItemByKey(screen, kVoiceChatMenuKey, &voiceItem);
    RemoveMenuItemByKey(screen, kAndroidKeyboardMenuKey, &keyboardItem);

    int restoreIndex = FindMenuItemIndexByAction(screen, restoreAction);
    if (restoreIndex < 0) {
        int count = *reinterpret_cast<int*>(
                reinterpret_cast<char*>(screen) + 0x24);
        if (fpsItem) {
            InsertMenuItemAt(screen, count, fpsItem);
            ++count;
        }
        if (voiceItem) {
            InsertMenuItemAt(screen, count, voiceItem);
            ++count;
        }
        if (keyboardItem) {
            InsertMenuItemAt(screen, count, keyboardItem);
        }
        return;
    }

    if (fpsItem) {
        InsertMenuItemAt(screen, restoreIndex, fpsItem);
        ++restoreIndex;
    }
    if (voiceItem) {
        InsertMenuItemAt(screen, restoreIndex, voiceItem);
        ++restoreIndex;
    }
    if (keyboardItem) {
        InsertMenuItemAt(screen, restoreIndex, keyboardItem);
    }
}

static void CMenuManager_DrawFrontEnd_hook(void* thiz)
{
    CMenuManager_DrawFrontEnd(thiz);
}

static void CMenuManager_Process_hook(void* thiz)
{
    CMenuManager_Process(thiz);
}

static void MobileMenu_Update_hook(void* thiz)
{
    MobileMenu_Update(thiz);
}

static void MobileMenu_Render_hook(void* thiz)
{
    if (g_skipMobileMenuRenderOnce) {
        g_skipMobileMenuRenderOnce = false;
        return;
    }
    bool wasInMobileMenu = g_inMobileMenuRender;
    g_inMobileMenuRender = true;
    MobileMenu_Render(thiz);
    g_inMobileMenuRender = wasInMobileMenu;
}

static void HideGameOptionsFromMenu()
{
    if (g_gameOptionsHidden) {
        return;
    }

    auto** settingsPtr = reinterpret_cast<MobileSettings**>(
            g_libGTASA + (VER_x32 ? 0x00679A3C : 0x851498));
    if (!settingsPtr || !*settingsPtr) {
        return;
    }

    MobileSettings* settings = *settingsPtr;
    if (MS_FrameLimiter < MS_MAX) {
        settings[MS_FrameLimiter].visible = false;
    }
    if (MS_Traffic < MS_MAX) {
        settings[MS_Traffic].visible = false;
    }

    g_gameOptionsHidden = true;
}

static bool IsLikelyAsciiKey(const char* key)
{
    if (!key) {
        return false;
    }
    for (int i = 0; i < 16; ++i) {
        char c = key[i];
        if (c == '\0') {
            return i > 0;
        }
        if (c < 0x20 || c > 0x7E) {
            return false;
        }
    }
    return true;
}

static void ToLowerAscii(const char* src, char* dst, size_t dstSize)
{
    if (!dstSize) {
        return;
    }
    size_t out = 0;
    if (src) {
        for (; src[out] && out + 1 < dstSize; ++out) {
            char c = src[out];
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c + ('a' - 'A'));
            }
            dst[out] = c;
        }
    }
    dst[out] = '\0';
}

static void Utf16ToLowerAscii(const uint16_t* text, char* dst, size_t dstSize)
{
    if (!dstSize) {
        return;
    }
    size_t out = 0;
    for (size_t i = 0; text && text[i] && out + 1 < dstSize; ++i) {
        uint16_t ch = text[i];
        if (ch == u'~') {
            ++i;
            while (text[i] && text[i] != u'~') {
                ++i;
            }
            continue;
        }
        if (ch < 0x80) {
            char c = static_cast<char>(ch);
            if (c >= 'A' && c <= 'Z') {
                c = static_cast<char>(c + ('a' - 'A'));
            }
            dst[out++] = c;
        }
    }
    dst[out] = '\0';
}

static bool ShouldHideGameOption(void* item)
{
    if (!item) {
        return false;
    }

    int32_t settingIndex = *reinterpret_cast<int32_t*>(
            reinterpret_cast<char*>(item) + 0x10);
    if (settingIndex == MS_FrameLimiter || settingIndex == MS_Traffic || settingIndex == MS_Language) {
        return true;
    }

    const char* key = *reinterpret_cast<const char**>(
            reinterpret_cast<char*>(item) + 0x8);
    if (IsLikelyAsciiKey(key)) {
        char keyLower[32]{};
        ToLowerAscii(key, keyLower, sizeof(keyLower));
        if (strstr(keyLower, "frame") || strstr(keyLower, "framelim") ||
            strstr(keyLower, "fmlim") || strstr(keyLower, "traffic") ||
            strstr(keyLower, "traf") || strstr(keyLower, "language") ||
            strstr(keyLower, "lang") || strstr(keyLower, "social") ||
            strstr(keyLower, "club") || strstr(keyLower, "signin")) {
            return true;
        }
    }

    if (!MenuSelection_GetTitle) {
        return false;
    }
    const uint16_t* title = MenuSelection_GetTitle(item);
    if (!title) {
        return false;
    }

    char titleLower[128]{};
    Utf16ToLowerAscii(title, titleLower, sizeof(titleLower));
    if (strstr(titleLower, "frame limiter")) {
        return true;
    }
    if (strstr(titleLower, "traffic mode") || strstr(titleLower, "traffic")) {
        return true;
    }
    if (strstr(titleLower, "language")) {
        return true;
    }
    if (strstr(titleLower, "social club") || strstr(titleLower, "sign in") ||
        strstr(titleLower, "signin")) {
        return true;
    }
    return false;
}

static void SelectScreen_AddItem_hook(void* screen, void* item)
{
    if (!screen || !item) {
        if (SelectScreen_AddItem_orig) {
            SelectScreen_AddItem_orig(screen, item);
        }
        return;
    }
#if !VER_x32
    void* action = *reinterpret_cast<void**>(
            reinterpret_cast<char*>(item) + 0x10);
    if (action == reinterpret_cast<void*>(g_libGTASA + kSettingsScreenOnLanguageOptions) ||
        action == reinterpret_cast<void*>(g_libGTASA + kSelectScreenOnSocialClub)) {
        return;
    }
#endif
    if (ShouldHideGameOption(item)) {
        return;
    }
    if (SelectScreen_AddItem_orig) {
        SelectScreen_AddItem_orig(screen, item);
    }
}

static bool IsMoneyText(const uint16_t* text)
{
    if (!text) {
        return false;
    }

    int idx = 0;
    if (text[idx] == '-') {
        idx++;
    }
    return text[idx] == '$';
}

static bool IsHudClockText(const uint16_t* text)
{
    if (!text) {
        return false;
    }

    if (text[0] < '0' || text[0] > '9') return false;
    if (text[1] < '0' || text[1] > '9') return false;
    if (text[2] != ':') return false;
    if (text[3] < '0' || text[3] > '9') return false;
    if (text[4] < '0' || text[4] > '9') return false;
    if (text[5] != 0) return false;

    return true;
}

static bool ShouldHideHudClock(float posX, float posY, const uint16_t* text)
{
    if (!IsHudClockText(text)) {
        return false;
    }

    if (!RsGlobal) {
        return true;
    }

    const float screenW = static_cast<float>(RsGlobal->maximumWidth);
    const float screenH = static_cast<float>(RsGlobal->maximumHeight);
    if (screenW <= 0.0f || screenH <= 0.0f) {
        return true;
    }

    const bool inTopRight = (posX >= screenW * 0.6f) && (posY <= screenH * 0.25f);
    return inTopRight;
}

static void CFont_PrintString_hook(float posX, float posY, uint16_t* text)
{
    if (ShouldHideHudClock(posX, posY, text)) {
        return;
    }

    CFont_PrintString(posX, posY, text);
}

uint8_t byteInternalPlayer = 0;
CPedGTA* dwCurPlayerActor = 0;
uint8_t byteCurPlayer = 0;

extern "C" uintptr_t get_lib()
{
    return g_libGTASA;
}
// 0.3.7
PLAYERID FindPlayerIDFromGtaPtr(CEntityGTA* pEntity)
{
    if (pEntity == nullptr) return INVALID_PLAYER_ID;

    CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
    CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();

    PLAYERID PlayerID = pPlayerPool->FindRemotePlayerIDFromGtaPtr((CPedGTA*)pEntity);
    if (PlayerID != INVALID_PLAYER_ID) return PlayerID;

    VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr((CVehicleGTA*)pEntity);
    if (VehicleID != INVALID_VEHICLE_ID)
    {
        for (PLAYERID i = 0; i < MAX_PLAYERS; i++)
        {
            CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(i);
            if (pRemotePlayer && pRemotePlayer->CurrentVehicleID() == VehicleID) {
                return i;
            }
        }
    }

    return INVALID_PLAYER_ID;
}
// 0.3.7
PLAYERID FindActorIDFromGtaPtr(CPedGTA* pPed)
{
    if (pPed) {
        return pNetGame->GetActorPool()->FindIDFromGtaPtr(pPed);
    }

    return INVALID_PLAYER_ID;
}

/* =============================================================================== */

void RenderEffects() {
//	RenderEffects();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0059DA40 + 1 : 0x6C1D6C));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005BE914 + 1 : 0x6E2FB4));
//    CRopes::Render();
//    CGlass::Render();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005A6BC8 + 1 : 0x6CA5D0));
    CVisibilityPlugins::RenderReallyDrawLastObjects();

    // --- FIX for square light coronas ---
    // Restore the additive blend state which might be corrupted by UI rendering.
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    CCoronas::Render();



    // FIXME
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    auto g_fx = *(uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00820520 : 0xA062A8));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00363DF0 + 1 : 0x433F54), &g_fx, TheCamera.m_pRwCamera, false);

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005CBBAC + 1 : 0x6F054C));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0059BF84 + 1 : 0x6C0268));
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005A1C38 + 1 : 0x6C552C));
    //   CClouds::VolumetricCloudsRender();
////    if (CHeli::NumberOfSearchLights || CTheScripts::NumberOfScriptSearchLights) {
////        CHeli::Pre_SearchLightCone();
////        CHeli::RenderAllHeliSearchLights();
////        CTheScripts::RenderAllSearchLights();
////        CHeli::Post_SearchLightCone();
////    }
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005E3390 + 1 : 0x708DF0));
////    if (CReplay::Mode != MODE_PLAYBACK && !CPad::GetPad(0)->DisablePlayerControls) {
////        FindPlayerPed()->DrawTriangleForMouseRecruitPed();
////    }
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005C0B14 + 1 : 0x6E50CC));
//    //CVehicleRecording::Render();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B19D0 + 1 : 0x6D6068));
//    //CRenderer::RenderFirstPersonVehicle();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B5F78 + 1 : 0x6DA2B8));

    //DebugModules::Render3D();
}

#include "java/Hud.h"
#include "game/weapons/Weapon.h"

void Render2dStuff()
{

    if(VER_x32) {
        *(uint8_t*)(g_libGTASA+0x819D88) = 0;
    } else {
        *(uint8_t*)(g_libGTASA+0x9FF3A8) = 0;
    }

    if( CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x001BB7F4 + 1 : 0x24EA90)) ) // emu_IsAltRenderTarget()
        CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x001BC20C + 1 : 0x24F5B8)); // emu_FlushAltRenderTarget()

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, RWRSTATE(rwRENDERSTATENARENDERSTATE));
    RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));

    const bool isPaused = CTimer::GetIsPaused();
    const bool isTabVisible = IsJavaTabListVisible();
    if (isTabVisible) {
        const uint32_t now = CTimer::GetTimeInMS();
        if (g_nextTabRefreshMs == 0 || now >= g_nextTabRefreshMs) {
            RefreshJavaTabList();
            g_nextTabRefreshMs = now + 500;
        }
    } else {
        g_nextTabRefreshMs = 0;
    }
    if (!isPaused && !isTabVisible) {
        CHook::CallFunction<void>("_ZN4CHud4DrawEv");
        //	GPS::Draw();
        //
        ((void(*)(bool) )(g_libGTASA + (VER_x32 ? 0x002B0BD8 + 1 : 0x36FB00)) )(false); // CTouchInterface::DrawAll
    }

    if (!Weapon::ms_bTakePhoto) {
        if (CHud::bIsShow) {
            CHud::UpdateHudInfo();

            CWidgetGta *v3 = CTouchInterface::m_pWidgets[0xA1];
            if (v3) {
                v3->m_fOriginX = CHud::radarPos.x;
                v3->m_fOriginY = CHud::radarPos.y;

                v3->m_fScaleX = CHud::radarSize;
                v3->m_fScaleY = CHud::radarSize;
            }
            // call CHud::DrawRadar
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x437B0D : 0x51CFF0)))();
        }
    }

    CHook::CallFunction<void>("_Z12emu_GammaSeth", 1);

    ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x0054BDD4 + 1 : 0x66B678)))(1u); // CMessages::Display - gametext
    ((void (*)(bool)) (g_libGTASA + (VER_x32 ? 0x005A9120 + 1 : 0x6CCEA0)))(1u); // CFont::RenderFontBuffer
    CHook::CallFunction<void>("_Z12emu_GammaSeth", 0);

    if(pNetGame)
    {
        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if(pTextDrawPool)
        {
            pTextDrawPool->Draw();

            // --- FIX for textdraw texture corruption ---
            // Immediately unbind the texture raster after drawing the textdraws.
            // This prevents textures from sprite-based textdraws from "leaking"
            // to other UI elements that are rendered next.
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        }
    }

    if (pUI) pUI->render();

    // --- FIX for texture corruption (shadows, lights, etc.) ---
    // After rendering the UI, unbind the last used texture raster AND the current texture dictionary.
    // This prevents UI textures and TXDs from "leaking" into the 3D world rendering in the next frame.
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

    // Restore 3D-friendly render states for the next frame. If 2D/UI left depth/cull/alpha
    // disabled, the next world render can flicker/pop objects when the camera moves.
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(FALSE));
    RwRenderStateSet(rwRENDERSTATESRCBLEND, RWRSTATE(rwBLENDSRCALPHA));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
    RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLBACK));
}

int g_iCounterVehicleCamera = 0;
int (*CPad__CycleCameraModeDownJustDown)(void*);
int CPad__CycleCameraModeDownJustDown_hook(void* thiz)
{
    CPedGTA* pPed = GamePool_FindPlayerPed();
    if (!pPed)
        return 0;

    static uint32_t lastTick = GetTickCount();
    bool bPressed = false;

    if (bIsTouchCameraButt && GetTickCount() - lastTick >= 250)
    {
        bIsTouchCameraButt = false;
        bPressed = true;
        lastTick = GetTickCount();
    }

    if (pPed->bInVehicle)
    {
        if (bPressed)
        {
            g_iCounterVehicleCamera++;
        }

        if (g_iCounterVehicleCamera == 6)
        {
            CFirstPersonCamera::SetEnabled(true);
            return 0;
        }
        else if (g_iCounterVehicleCamera >= 7)
        {
            g_iCounterVehicleCamera = 0;
            CFirstPersonCamera::SetEnabled(false);
            return 1;
        }
        else
        {
            CFirstPersonCamera::SetEnabled(false);
        }

        return bPressed;
    }

    return 0;
}

/* =============================================================================== */
#include "game/CGPS.hpp"
extern GPS* pGPS;

int (*CRadar__SetCoordBlip)(int r0, float X, float Y, float Z, int r4, int r5, char *name);
int CRadar__SetCoordBlip_hook(int r0, float X, float Y, float Z, int r4, int r5, char *name)
{
    if (pNetGame && !strncmp(name, "CODEWAY", 7))
    {
        float findZ = CWorld::FindGroundZForCoord(X, Y) + 1.5f;
        CVector pos = { X, Y, findZ - 1.5f };


        GPS::Set(pos, true);
        RakNet::BitStream bsSend;
        bsSend.Write(X);
        bsSend.Write(Y);
        bsSend.Write(findZ);
        pNetGame->GetRakClient()->RPC(&RPC_MapMarker, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
    }

    return CRadar__SetCoordBlip(r0, X, Y, Z, r4, r5, name);
}

/* =============================================================================== */

void(*CRadar_DrawRadarGangOverlay)(uint32_t unk);
void CRadar_DrawRadarGangOverlay_hook(uint32_t unk)
{
    if (pNetGame)
    {
        CGangZonePool *pGangZonePool = pNetGame->GetGangZonePool();
        if (pGangZonePool) {
            pGangZonePool->Draw(unk);
        }
    }
}

/* =============================================================================== */

typedef struct {
    CVector     vecPosObject;
    CQuaternion m_qRotation;
    int32       wModelIndex;
    union {
        struct { // CFileObjectInstanceType
            uint32 m_nAreaCode : 8;
            uint32 m_bRedundantStream : 1;
            uint32 m_bDontStream : 1; // Merely assumed, no countercheck possible.
            uint32 m_bUnderwater : 1;
            uint32 m_bTunnel : 1;
            uint32 m_bTunnelTransition : 1;
            uint32 m_nReserved : 19;
        };
        uint32 m_nInstanceType;
    };
    int32 m_nLodInstanceIndex; // -1 - without LOD model
} stLoadObjectInstance;
VALIDATE_SIZE(stLoadObjectInstance, (VER_x32 ? 0x28 : 0x28));

extern int iBuildingToRemoveCount;
extern REMOVEBUILDING_DATA BuildingToRemove[1000];

int (*CFileLoader__LoadObjectInstance)(stLoadObjectInstance *thiz);
int CFileLoader__LoadObjectInstance_hook(stLoadObjectInstance *thiz) {
    if (thiz) {
        if (iBuildingToRemoveCount >= 1) {
            for (int i = 0; i < iBuildingToRemoveCount; i++)
            {
                float fDistance = GetDistance(BuildingToRemove[i].vecPos, thiz->vecPosObject);
                if (fDistance <= BuildingToRemove[i].fRange) {
                    if (BuildingToRemove[i].dwModel == -1 || thiz->wModelIndex == (uint16_t) BuildingToRemove[i].dwModel) {
                        thiz->wModelIndex = 19300;
                        //thiz->vecPosObject = 0.0f;
                        break;
                    }
                }
            }
        }
    }

    return CFileLoader__LoadObjectInstance(thiz);
}

extern int iBuildingToRemoveCount;
extern std::list<REMOVE_BUILDING_DATA> RemoveBuildingData;
// Helper callback to check the geometry of every atomic within a clump.
RpAtomic* AtomicGeometryCheckCB(RpAtomic* atomic, void* data)
{
    bool* hasBadAtomic = (bool*)data;

    // If an atomic is found that has no geometry, flag it as bad.
    if (!atomic || !atomic->geometry)
    {
        *hasBadAtomic = true;
        return NULL; // Stop iterating, we found a bad one.
    }

    // This atomic is fine, continue to the next one.
    return atomic;
}

void (*CEntity_Render_orig)(CEntityGTA* pEntity);
void CEntity_Render_hook(CEntityGTA* pEntity)
{
    if (!pEntity || (uintptr_t)pEntity < 0x1000)
    {
        return;
    }

    CEntity_Render_orig(pEntity);
}
/* =============================================================================== */

/* =============================================================================== */

void (*CObject_Render)(CObjectGta* thiz);
void CObject_Render_hook(CObjectGta* thiz)
{
    if (!thiz || !thiz->m_pRwObject) return;

    CObjectGta *object = thiz;
    if(pNetGame && object != 0)
    {
        CObject *pObject = pNetGame->GetObjectPool()->FindObjectFromGtaPtr(object);
        if(pObject && pObject->m_pEntity)
        {
            RwObject* rwObject = (RwObject*)pObject->m_pEntity->m_pRwObject;
            if(rwObject)
            {
                // SetObjectMaterial
                if(pObject->m_bHasMaterial || pObject->m_bHasMaterialText)
                {
                    RwFrameForAllObjects((RwFrame*)rwObject->parent, (RwObject *(*)(RwObject *, void *))ObjectMaterialCallBack, pObject);
                    //RpAtomic* atomic = (RpAtomic*)object->m_pRwAtomic;
                    //RpGeometryForAllMaterials(atomic->geometry, ObjectMaterialCallBack, (void*)pObject);
                }
                // SetObjectMaterialText
                if(pObject->m_bHasMaterialText)
                {
                    RwFrameForAllObjects((RwFrame*)rwObject->parent, (RwObject *(*)(RwObject *, void *))ObjectMaterialTextCallBack, pObject);
                    //RpAtomic* atomic = (RpAtomic*)object->m_pRwAtomic;
                    //RpGeometryForAllMaterials(atomic->geometry, ObjectMaterialTextCallBack, (void*)pObject);
                }
            }


        }

        CObject_Render(object);
    }

    //((void (*)(void))(g_libGTASA + (VER_x32 ? 0x005D1F98 + 1 : 0x6F6664)))();
    //((void (*)(void))(g_libGTASA + 0x5D1F5C + 1))();
}

/*((void (*)(void))(g_libGTASA + 0x5D1F48 + 1))();
				CObject_Render(thiz);
				// ActivateDirectional
				((void (*)(void))(g_libGTASA + 0x5D1F5C + 1))();*/
/* =============================================================================== */

/* =============================================================================== */

bool NotifyEnterVehicle(CVehicleGTA *_pVehicle)
{
    if(!pNetGame) {
        return false;
    }

    CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
    if(!pVehiclePool) {
        return false;
    }

    CVehicle *pVehicle = nullptr;
    VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr(_pVehicle);

    if(VehicleID <= 0 || VehicleID >= MAX_VEHICLES) {
        return false;
    }

    if(!pVehiclePool->GetSlotState(VehicleID)) {
        return false;
    }

    pVehicle = pVehiclePool->GetAt(VehicleID);
    if(!pVehicle) {
        return false;
    }

    CLocalPlayer *pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

    if(pLocalPlayer) {
        FLog("Vehicle ID: %d", VehicleID);
        pLocalPlayer->SendEnterVehicleNotification(VehicleID, false);
    }

    return true;
}

int (*TaskEnterVehicle)(uintptr_t a1, uintptr_t a2);
int TaskEnterVehicleHook(uintptr_t a1, uintptr_t a2)
{
    if(!NotifyEnterVehicle((CVehicleGTA*)a1)) {
        return false;
    }

    // CTask::operator new
    uintptr_t pTask = ((uintptr_t (*)(void))(g_libGTASA + (VER_x32 ? 0x4D6A70:0x5D7414)))();

    // CTaskComplexEnterCarAsDriver::CTaskComplexEnterCarAsDriver
    ((void (__fastcall *)(uintptr_t, uintptr_t))(g_libGTASA + (VER_x32 ? 0x4F6FE0:0x6007E0)))(pTask, a1);

    // CTaskManager::SetTask
    ((int (__fastcall *)(uintptr_t, uintptr_t, int, int))(g_libGTASA + (VER_x32 ? 0x53397A:0x64E084)))(a2, pTask, 3, 0);

    return true;
}

void (*CTaskComplexLeaveCar)(uintptr_t** thiz, CVehicleGTA* pVehicle, int iTargetDoor, int iDelayTime, bool bSensibleLeaveCar, bool bForceGetOut);
void CTaskComplexLeaveCar_hook(uintptr_t** thiz, CVehicleGTA* pVehicle, int iTargetDoor, int iDelayTime, bool bSensibleLeaveCar, bool bForceGetOut)
{
    uintptr_t dwRetAddr = 0;
    __asm__ volatile ("mov %0, lr" : "=r" (dwRetAddr));
    dwRetAddr -= g_libGTASA;

    if (dwRetAddr == 0x409A42+1 || dwRetAddr == 0x40A818+1)
    {
        if (pNetGame)
        {
            if ((CVehicleGTA*)GamePool_FindPlayerPed()->pVehicle == pVehicle)
            {
                CVehiclePool* pVehiclePool = pNetGame->GetVehiclePool();
                VEHICLEID VehicleID = pVehiclePool->FindIDFromGtaPtr((CVehicleGTA*)GamePool_FindPlayerPed()->pVehicle);
                if (VehicleID != INVALID_VEHICLE_ID)
                {
                    CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);
                    CLocalPlayer* pLocalPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
                    if (pVehicle && pLocalPlayer)
                    {
                        if (pVehicle->IsATrainPart())
                        {
                            RwMatrix mat = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();
                            pLocalPlayer->GetPlayerPed()->RemoveFromVehicleAndPutAt(mat.pos.x + 2.5f, mat.pos.y + 2.5f, mat.pos.z);
                        }
                        else
                        {
                            pLocalPlayer->SendExitVehicleNotification(VehicleID);
                        }
                    }
                }
            }
        }
    }

    (*CTaskComplexLeaveCar)(thiz, pVehicle, iTargetDoor, iDelayTime, bSensibleLeaveCar, bForceGetOut);
}

/* =============================================================================== */

static uint32_t (*CRadar__GetRadarTraceColor_orig)(uint32_t color, uint8_t bright, uint8_t friendly) = nullptr;
uint32_t CRadar__GetRadarTraceColor(uint32_t color, uint8_t bright, uint8_t friendly)
{
    if (color == 1004 || color == 1005 || color == 1006 || IsRadarColorCodeInRange(color)) {
        return TranslateColorCodeToRGBA(color);
    }

    // Keep GTA native behavior for all non-SA:MP radar blips.
    return CRadar__GetRadarTraceColor_orig
           ? CRadar__GetRadarTraceColor_orig(color, bright, friendly)
           : TranslateColorCodeToRGBA(color);
}

#if  0
static uint32_t (*CHudColours__GetIntColour_orig)(uint32_t colour_id) = nullptr;
uint32_t CHudColours__GetIntColour(uint32 colour_id)
{
    if (colour_id == 1004 || colour_id == 1005 || colour_id == 1006 || IsRadarColorCodeInRange(colour_id)) {
        return TranslateColorCodeToRGBA(colour_id);
    }

    return CHudColours__GetIntColour_orig
           ? CHudColours__GetIntColour_orig(colour_id)
           : TranslateColorCodeToRGBA(colour_id);
}
#else
static uint32_t (*CHudColours__GetIntColour_orig)(uintptr* thiz, uint32_t colour_id) = nullptr;
uint32_t CHudColours__GetIntColour(uintptr* thiz, uint32_t colour_id)
{
    if (colour_id == 1004 || colour_id == 1005 || colour_id == 1006 || IsRadarColorCodeInRange(colour_id)) {
        return TranslateColorCodeToRGBA(colour_id);
    }

    if (!thiz || reinterpret_cast<uintptr_t>(thiz) < 0x1000) {
        return TranslateColorCodeToRGBA(colour_id);
    }

    return CHudColours__GetIntColour_orig
           ? CHudColours__GetIntColour_orig(thiz, colour_id)
           : TranslateColorCodeToRGBA(colour_id);
}
#endif

/* =============================================================================== */

void (*AND_TouchEvent)(int type, int num, int posX, int posY);
void AND_TouchEvent_hook(int type, int num, int posX, int posY)
{
    //FLog("Touch event: type=%d, num=%d, pos=(%d, %d)", type, num, posX, posY);
    // imgui
    //bool bRet = pUI->OnTouchEvent(type, num, posX, posY);

    if (pGame->IsGamePaused())
        return AND_TouchEvent(type, num, posX, posY);

    if (pUI != nullptr)
    {
        switch (type)
        {
            case 2: // push
                pUI->touchEvent(ImVec2(posX, posY), TouchType::push);
                break;

            case 3: // move
                pUI->touchEvent(ImVec2(posX, posY), TouchType::move);
                break;

            case 1: // pop
                pUI->touchEvent(ImVec2(posX, posY), TouchType::pop);
                break;
        }

        if (pUI->keyboard()->visible() || pUI->dialog()->visible()) {
            AND_TouchEvent(1, 0, 0, 0);
            return;
        }
        else
        {
            if (pNetGame && pNetGame->GetTextDrawPool())
            {
                if (!pNetGame->GetTextDrawPool()->onTouchEvent(type, num, posX, posY)) {
                    return AND_TouchEvent(1, 0, 0, 0);
                }
            }
        }
    }

    if (pGame->IsGameInputEnabled())
        AND_TouchEvent(type, num, posX, posY);
    else
        AND_TouchEvent(1, 0, 0, 0);
}
/* =============================================================================== */

/* =============================================================================== */

/* =============================================================================== */

uint32_t (*CPed__GetWeaponSkill)(CPedGTA *thiz);
uint32_t CPed__GetWeaponSkill_hook(CPedGTA *thiz)
{
    bool bWeaponSkillStored = false;

    dwCurPlayerActor = thiz;
    byteInternalPlayer = CWorld::PlayerInFocus;
    byteCurPlayer = FindPlayerNumFromPedPtr(dwCurPlayerActor);

    if(dwCurPlayerActor && byteCurPlayer != 0 && CWorld::PlayerInFocus == 0)
    {
        GameStoreLocalPlayerSkills();
        GameSetRemotePlayerSkills(byteCurPlayer);
        bWeaponSkillStored = true;
    }

    // CPed::GetWeaponSkill
    uint32_t result = (( uint32_t (*)(CPedGTA *, uint32_t))(g_libGTASA+0x4A55E2+1))(thiz, thiz->m_aWeapons[thiz->m_nActiveWeaponSlot].dwType);

    if(bWeaponSkillStored)
    {
        GameSetLocalPlayerSkills();
        bWeaponSkillStored = false;
    }

    return result;
}

/* =============================================================================== */

extern CPlayerPed* g_pCurrentFiredPed;
extern BULLET_DATA* g_pCurrentBulletData;

extern int g_iLagCompensationMode;

void SendBulletSync(CVector* vecOrigin, CVector* a2, CColPoint *colPoint, CEntityGTA** ppEntity)
{
    CMatrix mat1, mat2;

    static BULLET_DATA bulletData;
    memset(&bulletData, 0, sizeof(BULLET_DATA));

    bulletData.vecOrigin.x = vecOrigin->x;
    bulletData.vecOrigin.y = vecOrigin->y;
    bulletData.vecOrigin.z = vecOrigin->z;

    bulletData.vecPos.x = colPoint->m_vecPoint.x;
    bulletData.vecPos.y = colPoint->m_vecPoint.y;
    bulletData.vecPos.z = colPoint->m_vecPoint.z;

    if (ppEntity)
    {
        CEntityGTA* pEntity = *ppEntity;
        if (pEntity)
        {
            if (g_iLagCompensationMode != 0)
            {
                bulletData.vecOffset.x = colPoint->m_vecPoint.x - pEntity->m_matrix->m_pos.x;
                bulletData.vecOffset.y = colPoint->m_vecPoint.y - pEntity->m_matrix->m_pos.y;
                bulletData.vecOffset.z = colPoint->m_vecPoint.z - pEntity->m_matrix->m_pos.z;
            }
            else
            {
                memset(&mat1, 0, sizeof(CMatrix));
                memset(&mat2, 0, sizeof(CMatrix));
                // RwMatrixOrthoNormalize
                auto entMat = pEntity->GetMatrix().ToRwMatrix();
                RwMatrixOrthoNormalize(reinterpret_cast<RwMatrix *>(&mat2), &entMat);
                // RwMatrixInvert
                Invert(mat1, mat2);

                ProjectMatrix(&bulletData.vecOffset, &mat1, &colPoint->m_vecPoint);
            }

            bulletData.pEntity = pEntity;
        }
        else bulletData.vecOffset = 0;
    }

    pGame->FindPlayerPed()->ProcessBulletData(&bulletData);
}

extern bool g_customFire;
/* 0.3.7 */
uint32_t (*CWeapon__FireInstantHit)(CWeapon *thiz, CPed *pFiringEntity, CVector *vecOrigin,
                                    CVector *muzzlePosn, CEntity *targetEntity,
                                    CVector *target, CVector *originForDriveBy, bool arg6,
                                    bool muzzle);

uint32_t CWeapon__FireInstantHit_hook(CWeapon *thiz, CPed *pFiringEntity, CVector *vecOrigin,
                                      CVector *muzzlePosn, CEntity *targetEntity,
                                      CVector *target, CVector *originForDriveBy, bool arg6,
                                      bool muzzle) {
    if (pNetGame &&
        pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()->m_pPed)        // CWeapon::Fire
    {
        if (pFiringEntity != reinterpret_cast<CPed*>(GamePool_FindPlayerPed()))
            return muzzle;

        if (pNetGame) {
            pNetGame->GetPlayerPool()->ApplyCollisionChecking();
        }

        if (pGame) {
            CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
            if (pPlayerPed)
                pPlayerPed->FireInstant();
        }

        if (pNetGame) {
            pNetGame->GetPlayerPool()->ResetCollisionChecking();
        }

        return muzzle;
    }

    return CWeapon__FireInstantHit(thiz, pFiringEntity, vecOrigin, muzzlePosn, targetEntity,
                                   target, originForDriveBy, arg6, muzzle);
}

bool g_bForceWorldProcessLineOfSight = false;
uint32_t (*CWeapon__ProcessLineOfSight)(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CPedGTA **ppEntity, CWeapon *pWeaponSlot, CPedGTA **ppEntity2, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7);
uint32_t CWeapon__ProcessLineOfSight_hook(CVector *vecOrigin, CVector *vecEnd, CVector *vecPos, CPedGTA **ppEntity, CWeapon *pWeaponSlot, CPedGTA **ppEntity2, bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    FLog("dwRetAddr CWeapon__ProcessLineOfSight_hook 0x%llx", dwRetAddr);
#if VER_x32
    if(dwRetAddr >= 0x005DC178 && dwRetAddr <= 0x005DD684)
        g_bForceWorldProcessLineOfSight = true;
#else
    if(dwRetAddr >= 0x701494 && dwRetAddr <= 0x702B18)
        g_bForceWorldProcessLineOfSight = true;
#endif

    return CWeapon__ProcessLineOfSight(vecOrigin, vecEnd, vecPos, ppEntity, pWeaponSlot, ppEntity2, b1, b2, b3, b4, b5, b6, b7);
}
uint32_t(*CWorld__ProcessLineOfSight)(CVector*, CVector*, CColPoint *colPoint, CEntityGTA**, bool, bool, bool, bool, bool, bool, bool, bool);
uint32_t CWorld__ProcessLineOfSight_hook(CVector* vecOrigin, CVector* vecEnd, CColPoint *colPoint, CEntityGTA** ppEntity,
                                         bool b1, bool b2, bool b3, bool b4, bool b5, bool b6, bool b7, bool b8)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    if(dwRetAddr == (VER_x32 ? 0x005dd0b0 + 1 : 0x70253C) || g_bForceWorldProcessLineOfSight)
    {
        g_bForceWorldProcessLineOfSight = false;
        //LOGI("CWorld_ProcessLineOfSight iLagCompensationMode: %d", g_iLagCompensationMode);
        static CVector vecPosPlusOffset;

        if (g_iLagCompensationMode != 2)
        {
            if (g_pCurrentFiredPed != pGame->FindPlayerPed())
            {
                if (g_pCurrentBulletData && g_pCurrentBulletData->pEntity)
                {
                    if (*(uintptr_t*)(g_pCurrentBulletData->pEntity) != g_libGTASA+(VER_x32 ? 0x667D18:0x8300A0)) // CPlaceable
                    {
                        if (g_iLagCompensationMode)
                        {
                            vecPosPlusOffset.x = g_pCurrentBulletData->pEntity->GetPosition().x + g_pCurrentBulletData->vecOffset.x;
                            vecPosPlusOffset.y = g_pCurrentBulletData->pEntity->GetPosition().y + g_pCurrentBulletData->vecOffset.y;
                            vecPosPlusOffset.z = g_pCurrentBulletData->pEntity->GetPosition().z + g_pCurrentBulletData->vecOffset.z;
                        }
                        else
                        {
                            //FLog("vecPosPlusOffset %f %f %f", vecPosPlusOffset.x, vecPosPlusOffset.y, vecPosPlusOffset.z);
                            //FLog("pEntity->GetMatrix().m_up %f %f %f", g_pCurrentBulletData->pEntity->GetMatrix().m_up.x, g_pCurrentBulletData->pEntity->GetMatrix().m_up.y, g_pCurrentBulletData->pEntity->GetMatrix().m_up.z);
                            //FLog("g_pCurrentBulletData->vecOffset %f %f %f", g_pCurrentBulletData->vecOffset.x, g_pCurrentBulletData->vecOffset.y, g_pCurrentBulletData->vecOffset.z);
                            ProjectMatrix((CVector*)&vecPosPlusOffset, &g_pCurrentBulletData->pEntity->GetMatrix(), &g_pCurrentBulletData->vecOffset);
                            //vecPosPlusOffset.x = pEntity->GetMatrix().m_up.x * g_pCurrentBulletData->vecOffset.z + pEntity->GetMatrix().m_forward.x * g_pCurrentBulletData->vecOffset.y + pEntity->GetMatrix().m_right.x * g_pCurrentBulletData->vecOffset.x + pEntity->GetMatrix().m_pos.x;
                            //vecPosPlusOffset.y = pEntity->GetMatrix().m_up.y * g_pCurrentBulletData->vecOffset.z + pEntity->GetMatrix().m_forward.y * g_pCurrentBulletData->vecOffset.y + pEntity->GetMatrix().m_right.y * g_pCurrentBulletData->vecOffset.x + pEntity->GetMatrix().m_pos.y;
                            //vecPosPlusOffset.z = pEntity->GetMatrix().m_up.z * g_pCurrentBulletData->vecOffset.z + pEntity->GetMatrix().m_forward.z * g_pCurrentBulletData->vecOffset.y + pEntity->GetMatrix().m_right.z * g_pCurrentBulletData->vecOffset.x + pEntity->GetMatrix().m_pos.z;
                        }

                        vecEnd->x = vecPosPlusOffset.x - vecOrigin->x + vecPosPlusOffset.x;
                        vecEnd->y = vecPosPlusOffset.y - vecOrigin->y + vecPosPlusOffset.y;
                        vecEnd->z = vecPosPlusOffset.z - vecOrigin->z + vecPosPlusOffset.z;
                    }
                }
            }
        }

        uint32_t result = CWorld__ProcessLineOfSight(vecOrigin, vecEnd, colPoint, ppEntity, b1, b2, b3, b4, b5, b6, b7, b8);

        if (g_iLagCompensationMode == 2)
        {
            if (g_pCurrentFiredPed == pGame->FindPlayerPed()) {
                SendBulletSync(vecOrigin, vecEnd, colPoint, ppEntity);
            }
            return result;
        }

        if (g_pCurrentFiredPed)
        {
            if (g_pCurrentFiredPed != pGame->FindPlayerPed())
            {
                if (g_pCurrentBulletData)
                {
                    if (g_pCurrentBulletData->pEntity == nullptr)
                    {
                        CPedGTA* pLocalPed = GamePool_FindPlayerPed();
                        if (*ppEntity == GamePool_FindPlayerPed() ||
                            pLocalPed->IsInVehicle() && *ppEntity == pLocalPed->pVehicle)
                        {
                            result = 0;
                            *ppEntity = nullptr;
                            colPoint->m_vecPoint.x = 0.0f;
                            colPoint->m_vecPoint.y = 0.0f;
                            colPoint->m_vecPoint.z = 0.0f;
                            return result;
                        }
                    }
                }
            }
            else {
                SendBulletSync(vecOrigin, vecEnd, colPoint, ppEntity);
            }
        }

        return result;
    }

    return CWorld__ProcessLineOfSight(vecOrigin, vecEnd, colPoint, ppEntity, b1, b2, b3, b4, b5, b6, b7, b8);
}
// 0.3.7
uint32_t(*CWeapon__FireSniper)(CWeapon* thiz, CPedGTA* pFiringEntity, CEntityGTA* victim, CVector* target);
uint32_t CWeapon__FireSniper_hook(CWeapon* thiz, CPedGTA* pFiringEntity, CEntityGTA* victim, CVector* target)
{
    if (pFiringEntity == GamePool_FindPlayerPed())
    {
        if (pGame)
        {
            CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
            if (pPlayerPed) {
                pPlayerPed->FireInstant();
            }
        }
    }

    return true;
}
// 0.3.7
bool(*CBulletInfo_AddBullet)(CEntityGTA* creator, int weaponType, CVector pos, CVector velocity);
bool CBulletInfo_AddBullet_hook(CEntityGTA* creator, int weaponType, CVector pos, CVector velocity)
{
    velocity.x *= 50.0f;
    velocity.y *= 50.0f;
    velocity.z *= 50.0f;

    CBulletInfo_AddBullet(creator, weaponType, pos, velocity);

    // CBulletInfo::Update
    CHook::CallFunction<void>("_ZN11CBulletInfo6UpdateEv");
    return true;
}

#pragma pack(push, 1)
struct stPedDamageResponse {
    CEntity *pEntity;
    float fDamage;
    int iBodyPart;
    int iWeaponType;
    bool bSpeak;
};
#pragma pack(pop)

extern float m_fWeaponDamages[43 + 1];

void onDamage(CPed *issuer, CPed *damaged) {
    if (!pNetGame) return;
    CPedGTA *pPedPlayer = GamePool_FindPlayerPed();
    if (issuer == (CPed*)pPedPlayer) {
        if (damaged != nullptr) {
            if (pNetGame->GetPlayerPool()->FindRemotePlayerIDFromGtaPtr((CPedGTA *) damaged) !=
                INVALID_PLAYER_ID) {
                CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
                CAMERA_AIM *caAim = pPlayerPool->GetLocalPlayer()->GetPlayerPed()->GetCurrentAim();

                CVector aim;
                aim.x = caAim->f1x;
                aim.y = caAim->f1y;
                aim.z = caAim->f1z;

                pPlayerPool->GetLocalPlayer()->SendBulletSyncData(
                        pPlayerPool->FindRemotePlayerIDFromGtaPtr((CPedGTA *) damaged),
                        BULLET_HIT_TYPE_PLAYER, aim);
            }
        }
    }
}

// 0.3.7
//void (*CPedDamageResponseCalculator__ComputeDamageResponse)(CPedDamageResponseCalculator* thiz, CPed* pPed, uintptr_t* a3, uint32_t a4);
//void CPedDamageResponseCalculator__ComputeDamageResponse_hook(CPedDamageResponseCalculator* thiz, CPed* pPed, uintptr_t *a3, uint32_t a4)
void
(*CPedDamageResponseCalculator__ComputeDamageResponse)(stPedDamageResponse *thiz, CEntity *pEntity,
                                                       uintptr_t pDamageResponse, bool bSpeak);

void CPedDamageResponseCalculator__ComputeDamageResponse_hook(stPedDamageResponse *thiz,
                                                              CEntity *pEntity,
                                                              uintptr_t pDamageResponse,
                                                              bool bSpeak) {
    if (thiz && pEntity) onDamage((CPed *) *(uintptr_t *) thiz, (CPed *) pEntity);
    int weaponid = thiz->iWeaponType;
    float fDamage;
    FLog("%d", fDamage);
    if (weaponid < 0 || weaponid >= static_cast<int>(std::size(m_fWeaponDamages))) {
        fDamage = thiz->fDamage;
    } else {
        fDamage = m_fWeaponDamages[weaponid];
    }

    int bodypart = thiz->iBodyPart;

    if (pNetGame) {
        CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
        if (pPlayerPool) {
            if (weaponid < 0 || weaponid > 255 || (weaponid > 54 && weaponid < 200) ||
                (weaponid > 201 && weaponid < 255))
                weaponid = 255; // suicide
            else if (weaponid == 18)
                weaponid = 37; // flamethower
            else if (weaponid == 35 || weaponid == 16)
                weaponid = 51; // explosion

            PLAYERID damagedid = pPlayerPool->FindRemotePlayerIDFromGtaPtr(
                    (CPedGTA *) thiz->pEntity); // отправитель урона
            PLAYERID issuerid = pPlayerPool->FindRemotePlayerIDFromGtaPtr(
                    (CPedGTA *) pEntity); // получатель

            PLAYERID byteLocalId = pPlayerPool->GetLocalPlayerID();

            // give player damage
            if ((CPedGTA *) thiz->pEntity == pGame->FindPlayerPed()->m_pPed) {
                if (issuerid != INVALID_PLAYER_ID) {
                    //CHUD::addGiveDamageNotify(issuerid, weaponid, fDamage);
                    pPlayerPool->GetLocalPlayer()->GiveTakeDamage(false, issuerid, fDamage,
                                                                  weaponid,
                                                                  bodypart);
                    FLog(OBFUSCATE("GiveDamage Name: %s, weaponid: %d, damage: %f"),
                         pPlayerPool->GetPlayerName(issuerid), weaponid, fDamage);
                }

                    // player take damage
                else if (damagedid != INVALID_PLAYER_ID && issuerid == INVALID_PLAYER_ID) {
                    pPlayerPool->GetLocalPlayer()->GiveTakeDamage(true, damagedid, fDamage,
                                                                  weaponid,
                                                                  bodypart);

                    char nick[MAX_PLAYER_NAME];
                    strcpy(nick, pPlayerPool->GetPlayerName(damagedid));

                    FLog(OBFUSCATE("TakeDamage %s, %d, %f"), pPlayerPool->GetPlayerName(damagedid),
                         weaponid, fDamage);

                    //CHUD::addTakeDamageNotify(pPlayerPool->GetPlayerName(damagedid), weaponid, fDamage);
                }
            } else {
                if (false) {
                    //CHUD::addGiveDamageNotify(issuerid, weaponid, fDamage);
                    pPlayerPool->GetLocalPlayer()->GiveTakeDamage(false, issuerid, fDamage,
                                                                  weaponid,
                                                                  bodypart);
                    FLog(OBFUSCATE("GiveDamage Name: %s, weaponid: %d, damage: %f"),
                         pPlayerPool->GetPlayerName(issuerid), weaponid, fDamage);
                }

                    // player take damage
                else if (damagedid != INVALID_PLAYER_ID && issuerid == INVALID_PLAYER_ID) {
                    pPlayerPool->GetLocalPlayer()->GiveTakeDamage(true, damagedid, fDamage,
                                                                  weaponid,
                                                                  bodypart);

                    char nick[MAX_PLAYER_NAME];
                    strcpy(nick, pPlayerPool->GetPlayerName(damagedid));

                    FLog(OBFUSCATE("TakeDamage %s, %d, %f"), pPlayerPool->GetPlayerName(damagedid),
                         weaponid, fDamage);

                    //CHUD::addTakeDamageNotify(pPlayerPool->GetPlayerName(damagedid), weaponid, fDamage);
                }
            }
            if ((CPedGTA *) thiz->pEntity == pGame->FindPlayerPed()->m_pPed) {
                CHud::UpdateHudInfo();
            }


            FLog(OBFUSCATE("%d, %f, %d, %d"), damagedid, fDamage, weaponid, bodypart);
        }
    }

    CPedDamageResponseCalculator__ComputeDamageResponse(thiz, pEntity, pDamageResponse, bSpeak);
/*

	if (thiz == nullptr || pPed == nullptr || a3 == nullptr) return;

    if (ComputeDamageResponse(thiz, pPed))
        return;

	CPedDamageResponseCalculator__ComputeDamageResponse(thiz, pPed, a3, a4);*/
}
void (*CRenderer_RenderEverythingBarRoads)();
void CRenderer_RenderEverythingBarRoads_hook() {

    CRenderer_RenderEverythingBarRoads();

    if (pNetGame) {
        CObjectPool* pObjectPool = pNetGame->GetObjectPool();
        if (pObjectPool) {
            static std::vector<OBJECTID> s_forceRenderIds;
            static uint32_t s_lastScanTick = 0;
            const uint32_t now = GetTickCount();

            if (now - s_lastScanTick >= 500) {
                s_forceRenderIds.clear();
                for (OBJECTID i = 0; i < MAX_OBJECTS; i++) {
                    CObject* pObject = pObjectPool->GetAt(i);
                    if (pObject && pObject->m_bForceRender) {
                        s_forceRenderIds.push_back(i);
                    }
                }
                s_lastScanTick = now;
            }

            if (!s_forceRenderIds.empty()) {
                for (const auto id : s_forceRenderIds) {
                    CObject* pObject = pObjectPool->GetAt(id);
                    if (!pObject || !pObject->m_bForceRender) {
                        continue;
                    }
                    // CEntity::PreRender
                    ((void (*)(CEntityGTA*))(*(void**)(pObject->m_pEntity + (VER_x32 ? 0x48:0x48*2))))(pObject->m_pEntity);

                    // CRenderer::RenderOneNonRoad
                    ((void (*)(CEntityGTA*))(g_libGTASA+ (VER_x32 ? 0x41030C + 1:0x4F56E0)))(pObject->m_pEntity);
                }
            }
        }
    }
}

#include "CFPSFix.h"
#include "ES2VertexBuffer.h"
#include "RQ_Commands.h"
#include "Pickups.h"
#include "TimeCycle.h"
#include "game/Pipelines/CustomCar/CustomCarEnvMapPipeline.h"
#include "game/Pipelines/CustomBuilding/CustomBuildingDNPipeline.h"
#include "COcclusion.h"
#include "RealTimeShadowManager.h"

CFPSFix g_fps;

void (*ANDRunThread)(void* a1);
void ANDRunThread_hook(void* a1)
{
    g_fps.PushThread(gettid());

    ANDRunThread(a1);
}

static constexpr float ar43 = 4.0f/3.0f;
float *ms_fAspectRatio;
void (*DrawCrosshair)(uintptr_t* thiz);

void DrawCrosshair_hook(uintptr_t* thiz)
{
    float save1 = CCamera::m_f3rdPersonCHairMultX;
    CCamera::m_f3rdPersonCHairMultX = 0.530f - (*ms_fAspectRatio - ar43) * 0.01125f;

    float save2 = CCamera::m_f3rdPersonCHairMultY;
    CCamera::m_f3rdPersonCHairMultY = 0.400f + (*ms_fAspectRatio - ar43) * 0.03600f;

    DrawCrosshair(thiz);

    CCamera::m_f3rdPersonCHairMultX = save1;
    CCamera::m_f3rdPersonCHairMultY = save2;
}

CVector& (*FindPlayerSpeed)(int a1);
CVector& FindPlayerSpeed_hook(int a1)
{
    uintptr_t dwRetAddr = 0;
    __asm__ volatile ("mov %0, lr":"=r" (dwRetAddr));
    dwRetAddr -= g_libGTASA;

    if(dwRetAddr == 0x43E1F6 + 1)
    {
        if(pNetGame)
        {
            CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
            if(pPlayerPed &&
               pPlayerPed->IsInVehicle() &&
               pPlayerPed->IsAPassenger())
            {
                CVector vec = CVector(-1.0f);
                return vec;
            }
        }
    }

    return FindPlayerSpeed(a1);
}

int iLastTouchedWidgetId = -1;

int iLastReleasedWidgetId = -1;

int (*CTouchInterface__IsReleased)(int iWidgetId, int iUnk, int iEnableWidget);
int CTouchInterface__IsReleased_hook(int iWidgetId, int iUnk, int iEnableWidget)
{
    uintptr_t dwRetAddr = 0;
    __asm__ volatile ("mov %0, lr" : "=r" (dwRetAddr));
    dwRetAddr -= g_libGTASA;

    int iReleased = CTouchInterface__IsReleased(iWidgetId, iUnk, iEnableWidget);
    if(iReleased && iEnableWidget)
    {
        iLastReleasedWidgetId = iWidgetId;
    }

    return iReleased;
}

#include "game/Models/ModelInfo.h"
#include "game/Models/PedModelInfo.h"
#include "game/Models/BaseModelInfo.h"

// FIX CRASH -- ABOLFAZL

bool IsPedModel(unsigned int iModelID);

RpClump* (*RpClumpForAllAtomics_orig)(RpClump*, RpAtomicCallBack, void*);

RpClump* RpClumpForAllAtomics_hook(RpClump* clump, RpAtomicCallBack callback, void* pData) {
    if (clump && clump->object.parent) {
        return RpClumpForAllAtomics_orig(clump, callback, pData);
    }
    return clump;
}
RpClump* (*RpAnimBlendClumpInit_orig)(RpClump* clump);
RpClump* RpAnimBlendClumpInit_hook(RpClump* clump)
{
    if (!clump) return nullptr;

    bool hasBadAtomic = false;
    RpClumpForAllAtomics(clump, AtomicGeometryCheckCB, &hasBadAtomic);

    if (hasBadAtomic) {
        return clump;
    }
    return RpAnimBlendClumpInit_orig(clump);
}

RpHAnimHierarchy* GetAnimHierarchyFromSkinClump(RpClump* clump); // Forward declaration
void (*RpAnimBlendClumpFillFrameArray_orig)(RpClump* clump, void** animBlendFrameData);
void RpAnimBlendClumpFillFrameArray_hook(RpClump* clump, void** animBlendFrameData)
{
    if (GetAnimHierarchyFromSkinClump(clump))
    {
        RpAnimBlendClumpFillFrameArray_orig(clump, animBlendFrameData);
    }
}

class CAnimBlendAssociation;
CAnimBlendAssociation* (*CAnimManager_AddAnimation_orig)(RpClump* clump, int assocGroupId, int animationId);
CAnimBlendAssociation* CAnimManager_AddAnimation_hook(RpClump* clump, int assocGroupId, int animationId)
{
    void* pAnimBlend = *reinterpret_cast<void**>((uintptr_t)clump + 0x18);

    if (pAnimBlend)
    {
        return CAnimManager_AddAnimation_orig(clump, assocGroupId, animationId);
    }

    return nullptr;
}
void (*CPed_SetModelIndex_orig)(CPedGTA* ped, uint32_t modelId);

void CPed_SetModelIndex_hook(CPedGTA* ped, uint32_t modelId)
{
    if (!ped || (uintptr_t)ped < 0x1000) return;

    if (!IsPedModel(modelId)) {
        modelId = 0;
    }
    CPed_SetModelIndex_orig(ped, modelId);
}

#include "game/Entity/CEntityGTA.h"

void (*TidyUpModelInfo2_orig)(CEntityGTA* pEntity, bool bForce);

void TidyUpModelInfo2_hook(CEntityGTA* pEntity, bool bForce)
{
    if (!pEntity || (uintptr_t)pEntity < 0x1000)
    {
        return; // If the entity is invalid, do nothing.
    }
    TidyUpModelInfo2_orig(pEntity, bForce);
}

// =======================================================================
// FIX CRASH -- ABOLFAZL
int (*CTextureDatabaseRuntime__GetEntry)(uintptr_t thiz, const char* a2, bool* a3);
int CTextureDatabaseRuntime__GetEntry_hook(uintptr_t thiz, const char* a2, bool* a3)
{
    if (!thiz)
    {
        return -1;
    }
    return CTextureDatabaseRuntime__GetEntry(thiz, a2, a3);
}

RwTexture* (*GetTexture_orig)(const char* name);
static RwTexture* FindTextureInDatabases(const char* name)
{
    static char* texdb[] = { "samp", "mobile", "txd", "gta3", "gta_int", "player", "menu" };
    static auto* registered = reinterpret_cast<TDBArray<TextureDatabaseRuntime*>*>(
            CHook::getSym("_ZN22TextureDatabaseRuntime10registeredE")
    );

    for (int i = 0; i < sizeof(texdb) / sizeof(texdb[0]); ++i)
    {
        uintptr_t pDatabaseHandle = CHook::CallFunction<uintptr_t>(
                "_ZN22TextureDatabaseRuntime11GetDatabaseEPKc",
                texdb[i]
        );
        if (!pDatabaseHandle)
        {
            continue;
        }

        bool alreadyRegistered = false;
        if (registered && registered->dataPtr && registered->numEntries)
        {
            for (unsigned int j = 0; j < registered->numEntries; ++j)
            {
                if (registered->dataPtr[j] == reinterpret_cast<TextureDatabaseRuntime*>(pDatabaseHandle))
                {
                    alreadyRegistered = true;
                    break;
                }
            }
        }

        if (!alreadyRegistered)
        {
            CHook::CallFunction<void>("_ZN22TextureDatabaseRuntime8RegisterEPS_", pDatabaseHandle);
        }

        uintptr_t pTexture = CHook::CallFunction<uintptr_t>(
                "_ZN22TextureDatabaseRuntime10GetTextureEPKc",
                name
        );
        if (pTexture)
        {
            auto* tex = reinterpret_cast<RwTexture*>(pTexture);
            ++tex->refCount;
            return tex;
        }

        if (!alreadyRegistered)
        {
            CHook::CallFunction<void>("_ZN22TextureDatabaseRuntime10UnregisterEPS_", pDatabaseHandle);
        }
    }

    return nullptr;
}


int (*CCustomRoadsignMgr_RenderRoadsignAtomic)(int a1, int a2);
int CCustomRoadsignMgr_RenderRoadsignAtomic_hook(int a1, int a2)
{
    if ( a1 )
        return CCustomRoadsignMgr_RenderRoadsignAtomic(a1, a2);
}

int (*_RwTextureDestroy)(int a1);
int _RwTextureDestroy_hook(int a1)
{
    int result; // r0

    if ( (unsigned int)(a1 + 1) >= 2 )
        result = _RwTextureDestroy(a1);
    else
        result = 0;
    return result;
}

int (*CPed_UpdatePosition)(CPedGTA* a1);
int CPed_UpdatePosition_hook(CPedGTA* a1)
{
    int result; // r0

    if ( GamePool_FindPlayerPed() == a1 )
        result = CPed_UpdatePosition(a1);
    return result;
}

void (*CCamera__Process)(uintptr_t thiz);
void CCamera__Process_hook(uintptr_t thiz)
{
    //if(pGame->GetCamera())
    //pGame->GetCamera()->Update();

    CCamera__Process(thiz);
}

extern CJavaWrapper* pJavaWrapper;
void (*MainMenuScreen__OnExit)();
void MainMenuScreen__OnExit_hook()
{
    pGame->bIsGameExiting = true;

    pNetGame->GetRakClient()->Disconnect(0);

    pJavaWrapper->exitGame();
}

void (*rqVertexBufferSelect)(unsigned int **result);
void rqVertexBufferSelect_hook(unsigned int **result)
{
    uint32_t buffer = *(uint32_t *)*result;
    *result += 4;
    if ( buffer )
    {
        glBindBuffer(34962, *(uint32_t *)(buffer + 8));
        *(uint32_t*)(g_libGTASA + 0x6B8AF0) = 0;
    }
    else
    {
        glBindBuffer(34962, 0);
    }
}

uintptr_t* (*rpMaterialListDeinitialize)(RpMaterialList* matList);
uintptr_t* rpMaterialListDeinitialize_hook(RpMaterialList* matList)
{
    if(!matList || !matList->materials)
        return nullptr;

    return rpMaterialListDeinitialize(matList);
}

void (*rqVertexBufferDelete)(unsigned int **result);
void rqVertexBufferDelete_hook(unsigned int **result)
{
    uint32_t* buffer = *(uint32_t **)*result;
    *result += 4;
    glDeleteBuffers(1, reinterpret_cast<const GLuint *>(buffer + 2));
    buffer[2] = 0;
    if ( buffer )
        (*(void (**)(uint32_t *))(*buffer + 4))(buffer);
}

void rotate_ped_if_local(unsigned int *a1, unsigned int *a2)
{
    if ( GamePool_FindPlayerPed() == (CPedGTA*)a2 )
        *(uint32_t *)(a2 + 0x560) = *a1;
}

void (*player_control_zelda)(unsigned int *a2, unsigned int *a3);
void player_control_zelda_hook(unsigned int *a2, unsigned int *a3)
{
    rotate_ped_if_local(a2, a3);
}

static inline bool IsValidPtr(const void* ptr)
{
    return ptr && reinterpret_cast<uintptr_t>(ptr) >= 0x1000;
}

int (*rxOpenGLDefaultAtomicAllInOneNode_orig)(void* node, const RxPipelineNodeParam* params);
int rxOpenGLDefaultAtomicAllInOneNode_hook(void* node, const RxPipelineNodeParam* params)
{
    if (!node || (uintptr_t)node < 0x1000 || !params || (uintptr_t)params < 0x1000)
    {
        return 0;
    }
    return rxOpenGLDefaultAtomicAllInOneNode_orig(node, params);
}

// 006778B0
int (*rxOpenGLDefaultAllInOneRenderCB_orig)(RwResEntry* resEntry, void* object, uint8_t type, uint32_t flags);
int rxOpenGLDefaultAllInOneRenderCB_hook(RwResEntry* resEntry, void* object, uint8_t type, uint32_t flags)
{
    if (!resEntry || (uintptr_t)resEntry < 0x1000)
    {
        return 0;
    }

    if (!object || (uintptr_t)object < 0x1000)
    {
        return 0;
    }

    if (!resEntry->owner || (uintptr_t)resEntry->owner < 0x1000)
    {
        return 0;
    }

    if (!resEntry->ownerRef || (uintptr_t)resEntry->ownerRef < 0x1000 || *resEntry->ownerRef != resEntry)
    {
        return 0;
    }

    if (type == rpATOMIC) {
        RpAtomic* atomic = static_cast<RpAtomic*>(object);
        if (!atomic || (uintptr_t)atomic < 0x1000)
        {
            return 0;
        }

        if (!atomic->geometry || (uintptr_t)atomic->geometry < 0x1000)
        {
            return 0;
        }

        RpMaterialList& matList = atomic->geometry->matList;
        const int numMaterials = matList.numMaterials;
        if (numMaterials < 0 || numMaterials > 128) {
            return 0;
        }
        if (numMaterials > 0 && !IsValidPtr(matList.materials)) {
            return 0;
        }

        for (int i = 0; i < numMaterials; ++i) {
            RpMaterial* material = matList.materials[i];
            if (!IsValidPtr(material)) {
                return 0;
            }

            RwTexture* texture = material->texture;
            if (!IsValidPtr(texture)) {
                material->texture = nullptr;
                continue;
            }

            RwRaster* raster = texture->raster;
            if (!IsValidPtr(raster)) {
                material->texture = nullptr;
                continue;
            }

            RwRaster* parent = raster->parent;
            if (!IsValidPtr(parent)) {
                raster->parent = raster;
            }
        }
    }

    return rxOpenGLDefaultAllInOneRenderCB_orig(resEntry, object, type, flags);
}

int (*rwOpenGLSetRenderStateNoExtras_orig)(RwRenderState state, void* value);
int rwOpenGLSetRenderStateNoExtras_hook(RwRenderState state, void* value)
{
    if (g_inMobileMenuRender && state == rwRENDERSTATETEXTURERASTER) {
        return 1;
    }
    if (state == rwRENDERSTATETEXTURERASTER && value && (uintptr_t)value < 0x1000)
    {
        value = nullptr;
    }
    if (state == rwRENDERSTATETEXTURERASTER && value) {
        auto* raster = reinterpret_cast<RwRaster*>(value);
        if (((reinterpret_cast<uintptr_t>(raster) & 0x7u) != 0u) || g_validRasters.find(raster) == g_validRasters.end()) {
            value = nullptr;
        }
    }
    return rwOpenGLSetRenderStateNoExtras_orig(state, value);
}

int (*rwOpenGLSetRenderState_orig)(RwRenderState state, void* value);
int rwOpenGLSetRenderState_hook(RwRenderState state, void* value)
{
    if (g_inMobileMenuRender && state == rwRENDERSTATETEXTURERASTER) {
        return 1;
    }
    if (state == rwRENDERSTATETEXTURERASTER && value && (uintptr_t)value < 0x1000)
    {
        value = nullptr;
    }
    if (state == rwRENDERSTATETEXTURERASTER && value) {
        auto* raster = reinterpret_cast<RwRaster*>(value);
        if (((reinterpret_cast<uintptr_t>(raster) & 0x7u) != 0u) || g_validRasters.find(raster) == g_validRasters.end()) {
            value = nullptr;
        }
    }
    if (state == rwRENDERSTATETEXTURERASTER && !value) {
        g_skipMobileMenuRenderOnce = true;
    }
    return rwOpenGLSetRenderState_orig(state, value);
}

void (*rwGLHandleBlend_orig)(RwRaster* raster);
void rwGLHandleBlend_hook(RwRaster* raster)
{
    if (g_inMobileMenuRender) {
        return;
    }
    if (!raster || (uintptr_t)raster < 0x1000)
    {
        return;
    }
    if ((reinterpret_cast<uintptr_t>(raster) & 0x7u) != 0u)
    {
        return;
    }
    if (g_validRasters.find(raster) == g_validRasters.end())
    {
        return;
    }
    rwGLHandleBlend_orig(raster);
}

static RwRaster* (*RwRasterCreate_orig)(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
static RwBool (*RwRasterDestroy_orig)(RwRaster* raster);
static RwRaster* (*RwRasterRead_orig)(const RwChar* filename);
static RwRaster* (*RwRasterReadMaskedRaster_orig)(const RwChar* filename, const RwChar* maskname);
static RwRaster* (*RwRasterSubRaster_orig)(RwRaster* subRaster, RwRaster* raster, RwRect* rect);

static RwRaster* RwRasterCreate_hook(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags)
{
    RwRaster* raster = RwRasterCreate_orig(width, height, depth, flags);
    if (raster) {
        g_validRasters.insert(raster);
    }
    return raster;
}

static RwBool RwRasterDestroy_hook(RwRaster* raster)
{
    if (raster) {
        g_validRasters.erase(raster);
    }
    return RwRasterDestroy_orig(raster);
}

static RwRaster* RwRasterRead_hook(const RwChar* filename)
{
    RwRaster* raster = RwRasterRead_orig(filename);
    if (raster) {
        g_validRasters.insert(raster);
    }
    return raster;
}

static RwRaster* RwRasterReadMaskedRaster_hook(const RwChar* filename, const RwChar* maskname)
{
    RwRaster* raster = RwRasterReadMaskedRaster_orig(filename, maskname);
    if (raster) {
        g_validRasters.insert(raster);
    }
    return raster;
}

static RwRaster* RwRasterSubRaster_hook(RwRaster* subRaster, RwRaster* raster, RwRect* rect)
{
    RwRaster* out = RwRasterSubRaster_orig(subRaster, raster, rect);
    if (out) {
        g_validRasters.insert(out);
    }
    return out;
}

// 00677CB4
int (*CCustomBuildingDNPipeline__CustomPipeRenderCB)(RwResEntry* resEntry, uintptr_t object, uint8_t type, uint32_t flags);
int CCustomBuildingDNPipeline__CustomPipeRenderCB_hook(RwResEntry* resEntry, uintptr_t object, uint8_t type, uint32_t flags)
{
    if(!resEntry || !flags || !object)
        return 0;

    return rxOpenGLDefaultAllInOneRenderCB_hook(resEntry, (void*)object, type, flags);
}

int (*EmuShader_Select)(uintptr_t *result);
int EmuShader_Select_hook(uintptr_t *result)
{
    int result1;
    if ( *result >= 0x1000 )
        return EmuShader_Select(result);
    return 0;
}

float float_4DD9E8;
float ms_fTimeStep;
float fMagic = 50.0f / 30.0f;
void (*CTaskSimpleUseGun__SetMoveAnim)(uintptr_t *thiz, uintptr_t *a2);
void CTaskSimpleUseGun__SetMoveAnim_hook(uintptr_t *thiz, uintptr_t *a2)
{
    ms_fTimeStep = *(float*)(g_libGTASA + 0x96B500);
    float_4DD9E8 = *(float*)(g_libGTASA + 0x4DD9E8);
    float_4DD9E8 = (fMagic) * (0.1f / ms_fTimeStep);
    CTaskSimpleUseGun__SetMoveAnim(thiz, a2);
}

int (*CAnimManager_UncompressAnimation)(int result);
int CAnimManager_UncompressAnimation_hook(int result)
{
    if ( result )
        return CAnimManager_UncompressAnimation(result);
    return 0;
}

void readVehiclesAudioSettings();

void (*CVehicleModelInfo__SetupCommonData)();

void CVehicleModelInfo__SetupCommonData_hook() {
    CVehicleModelInfo__SetupCommonData();
    readVehiclesAudioSettings();
}

extern VehicleAudioPropertiesStruct VehicleAudioProperties[20000];
static uintptr_t addr_veh_audio = (uintptr_t) &VehicleAudioProperties[0];

void (*CAEVehicleAudioEntity__GetVehicleAudioSettings)(uintptr_t thiz, int16_t a2, int a3);

void CAEVehicleAudioEntity__GetVehicleAudioSettings_hook(uintptr_t dest, int16_t a2, int ID) {
    memcpy((void *) dest, &VehicleAudioProperties[(ID - 400)], sizeof(VehicleAudioPropertiesStruct));
}

void (*CRadar_ClearBlip)(uint32_t a2);
void CRadar_ClearBlip_hook(uint32_t a2)
{
    uintptr_t dwRetAddr = 0;
    GET_LR(dwRetAddr);

    //LOGI("[CRadar::ClearBlip]: %d called from 0x%X", (uint16_t)a2, dwRetAddr);

    if ( (uint16_t)a2 > 249 )
    {
        LOGI("[CRadar::ClearBlip]: Invalid blip ID (%d) called from 0x%X", (uint16_t)a2, dwRetAddr);
    }
    else
    {
        CRadar_ClearBlip(a2);
    }
}

void (*CVisibilityPlugins_RenderEntity_orig)(CEntityGTA* entity, float alpha);
void CVisibilityPlugins_RenderEntity_hook(CEntityGTA* entity, float alpha)
{
    if (!entity || !entity->m_pRwObject)
        return;

    CVisibilityPlugins_RenderEntity_orig(entity, alpha);
}

RpAtomic* (*AtomicDefaultRenderCallBack_orig)(RpAtomic* atomic);
RpAtomic* AtomicDefaultRenderCallBack_hook(RpAtomic* atomic)
{
    if (!atomic || (uintptr_t)atomic < 0x1000)
    {
        return atomic;
    }
    if (!atomic->geometry || (uintptr_t)atomic->geometry < 0x1000)
    {
        return atomic;
    }
    return AtomicDefaultRenderCallBack_orig(atomic);
}

void ReadSettingFile();
void ApplyFPSPatch(uint8_t fps);
void (*NvUtilInit)();
void NvUtilInit_hook()
{
    FLog("NvUtilInit");

    NvUtilInit();

    g_pszStorage = (char*)(g_libGTASA + (VER_x32 ? 0x6D687C : 0x8B46A8)); // StorageRootBuffer

    ReadSettingFile();

    ApplyFPSPatch(999);
}

struct stFile
{
    int isFileExist;
    FILE *f;
};

char lastFile[123];
stFile* NvFOpen(const char* r0, const char* r1, int r2, int r3)
{
    strcpy(lastFile, r1);

    static char path[255]{};
    memset(path, 0, sizeof(path));

    sprintf(path, "%s%s", g_pszStorage, r1);

    // ----------------------------
    if(!strncmp(r1+12, "mainV1.scm", 10))
    {
        sprintf(path, "%sSAMP/main.scm", g_pszStorage);
        FLog("Loading %s", path);
    }
    // ----------------------------
    if(!strncmp(r1+12, "SCRIPTV1.IMG", 12))
    {
        sprintf(path, "%sSAMP/script.img", g_pszStorage);
        FLog("Loading script.img..");
    }
    // ----------------------------
    if(!strncmp(r1, "DATA/PEDS.IDE", 13))
    {
        sprintf(path, "%sSAMP/peds.ide", g_pszStorage);
        FLog("Loading peds.ide..");
    }
    // ----------------------------
    if(!strncmp(r1, "DATA/VEHICLES.IDE", 17))
    {
        sprintf(path, "%sSAMP/vehicles.ide", g_pszStorage);
        FLog("Loading vehicles.ide..");
    }

    if (!strncmp(r1, "DATA/GTA.DAT", 12))
    {
        sprintf(path, "%sSAMP/gta.dat", g_pszStorage);
        FLog("Loading gta.dat..");
    }

    if (!strncmp(r1, "DATA/HANDLING.CFG", 17))
    {
        sprintf(path, "%sSAMP/handling.cfg", g_pszStorage);
        FLog("Loading handling.cfg..");
    }

    if (!strncmp(r1, "DATA/WEAPON.DAT", 15))
    {
        sprintf(path, "%sSAMP/weapon.dat", g_pszStorage);
        FLog("Loading weapon.dat..");
    }

#if VER_x32
    auto *st = (stFile*)malloc(8);
#else
    auto *st = (stFile*)malloc(0x10);
#endif
    st->isFileExist = false;

    FLog("%s", path);
    FILE *f  = fopen(path, "rb");

    if(f)
    {
        st->isFileExist = true;
        st->f = f;
        return st;
    }
    else
    {
        FLog("NVFOpen hook | Error: file not found (%s)", path);
        free(st);
        return nullptr;
    }
}

bool g_bPlaySAMP = false;

void (*CCam__Process)(uintptr_t thiz);

void CCam__Process_hook(uintptr_t thiz)
{
    if (!CFirstPersonCamera::IsEnabled()) {
        CCam__Process(thiz);
        return;
    }

    CVector vecSavedSpeed;
    CVehicle* pVeh = nullptr;

    float v6 = *(float*)(g_libGTASA + (VER_x32 ? 0x6A9FD0 : 0x8855D4));

    if (pNetGame && (*(uint16_t*)(thiz + 14) == 16 || *(uint16_t*)(thiz + 14) == 18)) {
        if (auto playerPool = pNetGame->GetPlayerPool()) {
            if (auto localPlayer = playerPool->GetLocalPlayer()) {
                CPlayerPed* pPed = localPlayer->GetPlayerPed();
                CVehicleGTA* contactVeh = (CVehicleGTA*)pPed->GetEntityUnderPlayer();
                VEHICLEID vehicleId = pNetGame->GetVehiclePool()->FindIDFromGtaPtr(contactVeh);
                CVehicle* pVeh = pNetGame->GetVehiclePool()->GetAt(vehicleId);

                if (pVeh && pVeh->m_pVehicle) {
                    CVector vecSavedSpeed = pVeh->m_pVehicle->m_vecMoveSpeed;

                    pVeh->m_pVehicle->m_vecMoveSpeed.x *= 6.0f;
                    pVeh->m_pVehicle->m_vecMoveSpeed.y *= 6.0f;
                    pVeh->m_pVehicle->m_vecMoveSpeed.z *= 6.0f;

                    CCam__Process(thiz);

                    pVeh->m_pVehicle->m_vecMoveSpeed = vecSavedSpeed;

                    *(float*)(g_libGTASA + (VER_x32 ? 0x6A9FD0 : 0x8855D4)) = 200.0f;
                }
            }
        }
    }

    CCam__Process(thiz);

    if (pVeh && pVeh->m_pVehicle) {
        pVeh->m_pVehicle->m_vecMoveSpeed = vecSavedSpeed;
        *(float*)(g_libGTASA + (VER_x32 ? 0x6A9FD0 : 0x8855D4)) = v6;
    }

    if (*(uint16_t*)(thiz + 14) == 4 || *(uint16_t*)(thiz + 14) == 53) {
        if (auto playerPool = pNetGame->GetPlayerPool()) {
            if (auto localPlayer = playerPool->GetLocalPlayer()) {
                CPlayerPed* pPed = localPlayer->GetPlayerPed();
                if (pPed) {
#if VER_x32
                    *(uint32_t*)(g_libGTASA + 0x00951FA8 + 120) = 0xFFFFFFFF;
                    *(uint32_t*)(g_libGTASA + 0x00951FA8 + 124) = 0xFFFFFFFF;
                    *(uint8_t*)(g_libGTASA + 0x00951FA8 + 40) = 0;
#else
                    *(uint32_t*)(g_libGTASA + 0xBBA8D0 + 128) = 0xFFFFFFFFFFFFFFFFLL;
                    *(uint8_t*)(g_libGTASA + 0xBBA8D0 + 48) = 0;
#endif
                    CFirstPersonCamera::ProcessCameraOnFoot(thiz, pPed);
                }
            }
        }
    }

    if (*(uint16_t*)(thiz + 14) == 16 || *(uint16_t*)(thiz + 14) == 18) {
        if (auto playerPool = pNetGame->GetPlayerPool()) {
            if (auto localPlayer = playerPool->GetLocalPlayer()) {
                CPlayerPed* pPed = localPlayer->GetPlayerPed();
                if (pPed) {
#if VER_x32
                    *(uint32_t*)(g_libGTASA + 0x00951FA8 + 120) = 0xFFFFFFFF;
                    *(uint32_t*)(g_libGTASA + 0x00951FA8 + 124) = 0xFFFFFFFF;
                    *(uint8_t*)(g_libGTASA + 0x00951FA8 + 40) = 0;
#else
                    *(uint32_t*)(g_libGTASA + 0xBBA8D0 + 128) = 0xFFFFFFFFFFFFFFFFLL;
                    *(uint8_t*)(g_libGTASA + 0xBBA8D0 + 48) = 0;
#endif
                    CFirstPersonCamera::ProcessCameraInVeh(thiz, pPed, pVeh);
                }
            }
        }
    }
}


void MainMenu_OnStartSAMP()
{
    if(g_bPlaySAMP) return;

    //InitInMenu();
    pGame->StartGame();

    // StartGameScreen::OnNewGameCheck()
    (( void (*)())(g_libGTASA + (VER_x32 ? 0x002A7270 + 1 : 0x365EA0)))();

    g_bPlaySAMP = true;
}

unsigned int (*MainMenuScreen__Update)(uintptr_t thiz, float a2);
unsigned int MainMenuScreen__Update_hook(uintptr_t thiz, float a2)
{
    unsigned int ret = MainMenuScreen__Update(thiz, a2);
    HideGameOptionsFromMenu();
    MainMenu_OnStartSAMP();
    return ret;
}

void (*StartGameScreen__OnNewGameCheck)();
void StartGameScreen__OnNewGameCheck_hook()
{
    // отключить кнопку начать игру
    if(g_bPlaySAMP)
        return;

    StartGameScreen__OnNewGameCheck();
}

void (*CTaskSimpleUseGun__RemoveStanceAnims)(uintptr* thiz, void* ped, float a3);
void CTaskSimpleUseGun__RemoveStanceAnims_hook(uintptr* thiz, void* ped, float a3)
{
    if (!thiz || !ped)
        return;

    CTaskSimpleUseGun__RemoveStanceAnims(thiz, ped, a3);
}
int (*CCollision__ProcessVerticalLine)(float *a1, float *a2, int a3, int a4, int *a5, int a6, int a7, int a8);
int CCollision__ProcessVerticalLine_hook(float *a1, float *a2, int a3, int a4, int *a5, int a6, int a7, int a8)
{
    int result; // r0

    if (a3)
        result = CCollision__ProcessVerticalLine(a1, a2, a3, a4, a5, a6, a7, a8);
    else
        result = 0;
    return result;
}

int(*CUpsideDownCarCheck__IsCarUpsideDown)(int, int);
int CUpsideDownCarCheck__IsCarUpsideDown_hook(int a1, int a2)
{
    /* Passengers leave the vehicle out of fear if it overturns */

//	if (*(uintptr_t*)(a2 + 20))
//	{
//		return CUpsideDownCarCheck__IsCarUpsideDown(a1, a2);
//	}
    return 0;
}

int (*CTaskSimpleGetUp__ProcessPed)(uintptr_t* thiz, CPedGTA* ped);
int CTaskSimpleGetUp__ProcessPed_hook(uintptr_t* thiz, CPedGTA* ped)
{
    //return false;
    if(!ped)return 0;
    int res = 0;
    try {
        res = CTaskSimpleGetUp__ProcessPed(thiz, ped);
    }
    catch(...) {
        return 0;
    }

    return res;
}

int64 getmip()
{
    return 1;
}

uint64_t* RQCommand_rqSetAlphaTest(uint64_t *result)
{
    if (!result)
        return result;

    auto *q = reinterpret_cast<uint8_t*>(*result);
    if (!q)
        return result;

    const uint32_t enableOrFunc = *reinterpret_cast<uint32_t*>(q);
    const uint32_t refValue = *reinterpret_cast<uint32_t*>(q + 4);
    *result = reinterpret_cast<uint64_t>(q + 8);

    // Emulate alpha test state change when glAlphaFunc isn't available.
    if (enableOrFunc) {
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, RWRSTATE(rwALPHATESTFUNCTIONGREATER));
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(refValue));
    } else {
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, RWRSTATE(rwALPHATESTFUNCTIONALWAYS));
    }
    return result;
}

int64 GetInputType(void)
{
    return 0LL;
}

int(*CAnimBlendNode__FindKeyFrame)(int, float, int, int);
int CAnimBlendNode__FindKeyFrame_hook(int a1, float a2, int a3, int a4)
{
    if (*(uintptr_t*)(a1 + 16))
    {
        return CAnimBlendNode__FindKeyFrame(a1, a2, a3, a4);
    }
    else return 0;
}

RwFrame* CClumpModelInfo_GetFrameFromId_Post(RwFrame* pFrameResult, RpClump* pClump, int id)
{
    if (pFrameResult)
        return pFrameResult;

    uintptr_t calledFrom = 0;
    __asm__ volatile ("mov %0, lr" : "=r" (calledFrom));
    calledFrom -= g_libGTASA;

    if (calledFrom == 0x00515708                // CVehicle::SetWindowOpenFlag
        || calledFrom == 0x00515730             // CVehicle::ClearWindowOpenFlag
        || calledFrom == 0x00338698             // CVehicleModelInfo::GetOriginalCompPosition
        || calledFrom == 0x00338B2C)            // CVehicleModelInfo::CreateInstance
        return nullptr;

    for (uint i = 2; i < 40; i++)
    {
        RwFrame* pNewFrameResult = nullptr;
        uint     uiNewId = id + (i / 2) * ((i & 1) ? -1 : 1);

        pNewFrameResult = ((RwFrame * (*)(RpClump * pClump, int id))(g_libGTASA + (VER_2_1 ? 0x003856D0 : 0x00335CC0) + 1))(pClump, i);

        if (pNewFrameResult)
        {
            return pNewFrameResult;
        }
    }

    return nullptr;
}
RwFrame* (*CClumpModelInfo_GetFrameFromId)(RpClump*, int);
RwFrame* CClumpModelInfo_GetFrameFromId_hook(RpClump* a1, int a2)
{
    return CClumpModelInfo_GetFrameFromId_Post(CClumpModelInfo_GetFrameFromId(a1, a2), a1, a2);
}

void (*FxEmitterBP_c__Render)(uintptr_t* a1, int a2, int a3, float a4, char a5);
void FxEmitterBP_c__Render_hook(uintptr_t* a1, int a2, int a3, float a4, char a5)
{
    if(!a1 || !a2) return;
    uintptr_t* temp = *((uintptr_t**)a1 + 3);
    if (!temp)
    {
        return;
    }
    FxEmitterBP_c__Render(a1, a2, a3, a4, a5);
}

bool (*RwResourcesFreeResEntry)(void* entry);
bool RwResourcesFreeResEntry_hook(void* entry)
{
    bool result;
    if (entry) result = RwResourcesFreeResEntry(entry);
    else result = false;
    return result;
}
static uint32_t dwRLEDecompressSourceSize = 0;

size_t (*OS_FileRead)(OSFile a1, void *buffer, size_t numBytes);
size_t OS_FileRead_hook(OSFile a1, void *buffer, size_t numBytes)
{
    dwRLEDecompressSourceSize = numBytes;

    return OS_FileRead(a1, buffer, numBytes);
}
// Tracks the last texture block name loaded; useful for debugging RLE decode issues.
char g_iLastBlock[123] = {0};
int *(*LoadFullTexture)(TextureDatabaseRuntime *thiz, unsigned int a2);
int *LoadFullTexture_hook(TextureDatabaseRuntime *thiz, unsigned int a2)
{
    strcpy(g_iLastBlock, thiz->name);

    return LoadFullTexture(thiz, a2);
}
void (*RLEDecompress)(uint8_t* pDest, size_t uiDestSize, uint8_t const* pSrc, size_t uiSegSize, uint32_t uiEscape);
void RLEDecompress_hook(uint8_t* pDest, size_t uiDestSize, const uint8_t* pSrc, size_t uiSegSize, uint32_t uiEscape) {

    if (!pDest || !pSrc || uiDestSize == 0 || uiSegSize == 0) {
        // Обработка некорректных входных данных или размеров
        // Здесь можно сгенерировать исключение или вернуть код ошибки
        return;
    }

    const uint8_t* pTempSrc = pSrc;
    const uint8_t* const pEndOfDest = pDest + uiDestSize;
    const uint8_t* const pEndOfSrc = pSrc + dwRLEDecompressSourceSize; // Предполагается, что dwRLEDecompressSourceSize определено правильно

    try {
        while (pDest < pEndOfDest && pTempSrc < pEndOfSrc) {
            if (*pTempSrc == uiEscape) {
                if (pTempSrc + 1 >= pEndOfSrc || pTempSrc[1] == 0 || pTempSrc + 2 + uiSegSize > pEndOfSrc) {
                    // Обработка ошибки, неверное значение ucCurSeg или недостаточно данных в исходном буфере
                    //throw std::runtime_error("rled error 1");
                }

                uint8_t ucCurSeg = pTempSrc[1];
                while (ucCurSeg--) {
                    if (pDest + uiSegSize > pEndOfDest) {
                        // Обработка ошибки, недостаточно места в целевом буфере
                        //throw std::runtime_error("rled error 2");
                    }
                    memcpy(pDest, pTempSrc + 2, uiSegSize);
                    pDest += uiSegSize;
                }
                pTempSrc += 2 + uiSegSize;
            } else {
                if (pDest + uiSegSize > pEndOfDest || pTempSrc + uiSegSize > pEndOfSrc) {
                    // Обработка ошибки, недостаточно данных в исходном буфере или недостаточно места в целевом буфере
                    //throw std::runtime_error("rled error 3");
                }
                memcpy(pDest, pTempSrc, uiSegSize);
                pDest += uiSegSize;
                pTempSrc += uiSegSize;
            }
        }

        dwRLEDecompressSourceSize = 0;
    } catch (const std::exception& e) {
        FLog("%s", e.what());
    }
}

void (*CGame_Process)();
void CGame_Process_hook()
{
    if(pGame->bIsGameExiting)return;

    // Disable occlusion culling each frame to avoid objects popping out due to bad occluder data.
    COcclusion::NumOccludersOnMap = 0;

    CGame_Process();

    // Push LOD distances out to reduce visible popping when rotating the camera.
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    TheCamera.m_fLODDistMultiplier = 2.0f;
    TheCamera.GenerationDistMultiplier = 2.0f;

    if (pNetGame)
    {
        if(pGame && pGame->FindPlayerPed() && pUI && pUI->chat())
        {
            if(pGame->FindPlayerPed()->IsInVehicle())
            {
                // pUI->buttonpanel()->m_bH->setCaption("D/B");
            }
        }

        CObjectPool* pObjectPool = pNetGame->GetObjectPool();
        if (pObjectPool) {
            pObjectPool->Process();
            pObjectPool->ProcessMaterialText();
        }

        CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
        if (pTextDrawPool) {
            pTextDrawPool->SnapshotProcess();
        }
    }
}

float (*CDraw__SetFOV)(float thiz, float a2);
float CDraw__SetFOV_hook(float thiz, float a2)
{
    float tmp = (float)((float)((float)(*(float *)&*(float *)(g_libGTASA + (VER_x32 ? 0x00A26A90 : 0xCC7F00)) - 1.3333) * 11.0) / 0.44444) + thiz;
    if(tmp > 100) tmp = 100.0;
    *(float *)(g_libGTASA + (VER_x32 ? 0x006B1CB8 : 0x88E6BC)) = tmp;
    return thiz;
}

void(*CStreaming__Init2)();
void CStreaming__Init2_hook()
{
    CStreaming__Init2();
    constexpr uint32_t kStreamingMemoryMb = 512;
    constexpr uint32_t kStreamingMemoryBytes = kStreamingMemoryMb * 1024u * 1024u;
    CStreaming::ms_memoryAvailable = kStreamingMemoryBytes;
    *(uint32_t*)(g_libGTASA + (VER_x32 ? 0x00685FA0 : 0x85EBD8)) = kStreamingMemoryBytes;
}

int(*mpg123_param)(void* mh, int key, long val, int ZERO, double fval);
int mpg123_param_hook(void* mh, int key, long val, int ZERO, double fval)
{
    // 0x2000 = MPG123_SKIP_ID3V2
    // 0x200  = MPG123_FUZZY
    // 0x100  = MPG123_SEEKBUFFER
    // 0x40   = MPG123_GAPLESS
    return mpg123_param(mh, key, val | (0x2000 | 0x200 | 0x100 | 0x40), ZERO, fval);
}

#include "Widgets/TouchInterface.h"
#include "Widgets/WidgetGta.h"
#include "game/Mobile/MobileMenu/MobileMenu.h"

void InjectHooks()
{
    FLog("InjectHooks");
    CHook::Write(g_libGTASA + (VER_x32 ? 0x678954 : 0x84F2D0), &Scene);

#if !VER_x32 // mb all.. wtf crash x64?
    CHook::RET("_ZN11CPlayerInfo14LoadPlayerSkinEv");
    CHook::RET("_ZN11CPopulation10InitialiseEv");
#endif
    CCustomCarEnvMapPipeline::InjectHooks();
    CCamera::InjectHooks(); //
    CReferences::InjectHooks(); //
    CModelInfo::injectHooks(); //
    CTimer::InjectHooks(); //
    //cTransmission::InjectHooks(); //
    CAnimBlendAssociation::InjectHooks(); //
    //cHandlingDataMgr::InjectHooks(); //
    CPools::InjectHooks(); //
    CVehicleGTA::InjectHooks(); //
    CMatrixLink::InjectHooks(); //
    CMatrixLinkList::InjectHooks(); //
    CStreaming::InjectHooks();
    CPlaceable::InjectHooks(); //
    CMatrix::InjectHooks(); //
    CCollision::InjectHooks(); //
    //CIdleCam::InjectHooks(); //
    CTouchInterface::InjectHooks(); //
    CWidgetGta::InjectHooks();
    CEntityGTA::InjectHooks(); //
    CPhysical::InjectHooks(); //
    CAnimManager::InjectHooks(); //
    //CCarEnterExit::InjectHooks();
    CPlayerPedGta::InjectHooks(); //
    CTaskManager::InjectHooks(); //
    //CPedIntelligence::InjectHooks(); //
    CWorld::InjectHooks(); //
    CGame::InjectHooks();
    ES2VertexBuffer::InjectHooks();
    CRQ_Commands::InjectHooks();
    CTxdStore::InjectHooks();
    CVisibilityPlugins::InjectHooks();
    //CAdjustableHUD::InjectHooks();

    // new
    //CClouds::InjectHooks();
    //CWeather::InjectHooks();
    //RenderBuffer::InjectHooks();
    CTimeCycle::InjectHooks();
    CCoronas::InjectHooks();
    CDraw::InjectHooks();
    //CClock::InjectHooks();
    //CBirds::Init();
    CVehicleModelInfo::InjectHooks();
    //CPathFind::InjectHooks();
    CSprite2d::InjectHooks();
    //CFileLoader::InjectHooks();
    //CShadows::InjectHooks();
    CPickups::InjectHooks();
    CRenderer::InjectHooks();
    CStreamingInfo::InjectHooks();
    TextureDatabase::InjectHooks();
    TextureDatabaseEntry::InjectHooks();
    TextureDatabaseRuntime::InjectHooks();
    CCustomBuildingDNPipeline::InjectHooks();

    CMobileSettings::InjectHooks();
    CMobileMenu::InjectHooks();

    CRealTimeShadowManager::InjectHooks();
    CHook::Write(g_libGTASA+(VER_x32 ? 0xA41140 : 0xCE3EE8), &COcclusion::aOccluders);
    CHook::Write(g_libGTASA+(VER_x32 ? 0xA45790:0xCE8538), &COcclusion::NumOccludersOnMap);
}

void DataFix()
{
    CHook::UnFuck(g_libGTASA + 0x714003);
    *(char*)(g_libGTASA + 0x714003 + 12) = 'd';
    *(char*)(g_libGTASA + 0x714003 + 13) = 'x';
    *(char*)(g_libGTASA + 0x714003 + 14) = 't';

    CHook::UnFuck(g_libGTASA + 0x71406F);
    *(char*)(g_libGTASA + 0x71406F + 12) = 'd';
    *(char*)(g_libGTASA + 0x71406F + 13) = 'x';
    *(char*)(g_libGTASA + 0x71406F + 14) = 't';

    // ETC texture format -> force to DXT
    CHook::UnFuck(g_libGTASA + 0x714017);
    *(char*)(g_libGTASA + 0x714017 + 12) = 'd';
    *(char*)(g_libGTASA + 0x714017 + 13) = 'x';
    *(char*)(g_libGTASA + 0x714017 + 14) = 't';

    CHook::UnFuck(g_libGTASA + 0x71407F);
    *(char*)(g_libGTASA + 0x71407F + 12) = 'd';
    *(char*)(g_libGTASA + 0x71407F + 13) = 'x';
    *(char*)(g_libGTASA + 0x71407F + 14) = 't';

    // Uncompressed format -> force to DXT
    CHook::UnFuck(g_libGTASA + 0x713FB3);
    *(char*)(g_libGTASA + 0x713FB3 + 12) = 'd';
    *(char*)(g_libGTASA + 0x713FB3 + 13) = 'x';
    *(char*)(g_libGTASA + 0x713FB3 + 14) = 't';
}

RwFrame* (*RwFrameAddChild)(RwFrame *parent, RwFrame *child);

RwFrame* RwFrameAddChild_hook(RwFrame *parent, RwFrame *child)
{
    if (!parent || !child) return nullptr;

    return RwFrameAddChild(parent, child);
}
void CrashFix()
{
    CHook::InlineHook("_ZN11CAudioZones6UpdateEb7CVector", &CAudioZones__Update_hook, &CAudioZones__Update_orig);

    CHook::InlineHook(g_libGTASA + 0x26FE48, (uintptr_t)RwFrameAddChild_hook, (uintptr_t*)&RwFrameAddChild);

    CHook::InlineHook(g_libGTASA + 0x46A6F0, (uintptr_t)RpAnimBlendClumpFillFrameArray_hook, (uintptr_t*)&RpAnimBlendClumpFillFrameArray_orig);

    CHook::InlineHook(g_libGTASA + 0x46A7D4, (uintptr_t)RpAnimBlendClumpInit_hook, (uintptr_t*)&RpAnimBlendClumpInit_orig);

    CHook::InlineHook(g_libGTASA + 0x2CD73C,(uintptr_t)rxOpenGLDefaultAllInOneRenderCB_hook,(uintptr_t*)&rxOpenGLDefaultAllInOneRenderCB_orig);

    CHook::InlineHook(g_libGTASA + 0x2CB5E0,(uintptr_t)rxOpenGLDefaultAtomicAllInOneNode_hook,(uintptr_t*)&rxOpenGLDefaultAtomicAllInOneNode_orig);

    CHook::InlineHook(g_libGTASA + 0x2B9B4C,(uintptr_t)AtomicDefaultRenderCallBack_hook,(uintptr_t*)&AtomicDefaultRenderCallBack_orig);

    CHook::InlineHook(g_libGTASA + 0x2BA020,(uintptr_t)RpClumpForAllAtomics_hook,(uintptr_t*)&RpClumpForAllAtomics_orig);

    CHook::InlineHook(g_libGTASA + 0x466B64, (uintptr_t)CAnimManager_AddAnimation_hook, (uintptr_t*)&CAnimManager_AddAnimation_orig);

    CHook::InlineHook(g_libGTASA + 0x595998, (uintptr_t)CPed_SetModelIndex_hook, (uintptr_t*)&CPed_SetModelIndex_orig);

    CHook::InlineHook(g_libGTASA + 0x4D6A08, (uintptr_t)TidyUpModelInfo2_hook, (uintptr_t*)&TidyUpModelInfo2_orig);
}


void InstallSpecialHooks()
{
    InjectHooks();

    DataFix();

    CrashFix();

    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x6701D4 : 0x840708), &RLEDecompress_hook, &RLEDecompress);

    // CHook::Redirect("_ZN5CGame20InitialiseRenderWareEv", &CGame::InitialiseRenderWare);
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x6785FC : 0x84EC20), &StartGameScreen__OnNewGameCheck_hook, &StartGameScreen__OnNewGameCheck);

    CHook::InlineHook("_Z10NvUtilInitv", &NvUtilInit_hook, &NvUtilInit);

    CHook::InlineHook("_ZN22TextureDatabaseRuntime15LoadFullTextureEj", &LoadFullTexture_hook, &LoadFullTexture);

    CHook::RET("_ZN12CCutsceneMgr16LoadCutsceneDataEPKc"); // LoadCutsceneData
    CHook::RET("_ZN12CCutsceneMgr10InitialiseEv");			// CCutsceneMgr::Initialise

    CHook::Redirect("_Z7NvFOpenPKcS0_bb", &NvFOpen);

    CHook::InlineHook("_ZN14MainMenuScreen6UpdateEf", &MainMenuScreen__Update_hook, &MainMenuScreen__Update);

    CHook::RET("_ZN4CPed31RemoveWeaponWhenEnteringVehicleEi"); // CPed::RemoveWeaponWhenEnteringVehicle

    CHook::InlineHook("_Z11OS_FileReadPvS_i", &OS_FileRead_hook, &OS_FileRead);

    CHook::InlineHook("_ZN25CCustomBuildingDNPipeline18CustomPipeRenderCBEP10RwResEntryPvhj", &CCustomBuildingDNPipeline__CustomPipeRenderCB_hook, &CCustomBuildingDNPipeline__CustomPipeRenderCB);
}

#include <EGL/egl.h>
#include <GLES2/gl2.h>   // If using OpenGL ES 2.0 or 3.0

/**
 * Installs various hooks to modify the behavior of the game
 * This function redirects function calls and replaces them with custom implementations
 */

void (*CCoronas__Render_orig)();
void CCoronas__Render_hook()
{
    // Restore the correct render state for coronas
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

    CCoronas__Render_orig();
}
void (*CEntity_Render_orig_new)(CEntityGTA* pEntity);
void CEntity_Render_hook_new(CEntityGTA* pEntity)
{
    if (!pEntity || (uintptr_t)pEntity < 0x1000 || !pEntity->m_pRwObject)
    {
        return;
    }

    // Also, check the model info associated with the entity.
    CBaseModelInfo* modelInfo = CModelInfo::GetModelInfo(pEntity->m_nModelIndex);
    if (!modelInfo)
    {
        return;
    }
    CEntity_Render_orig_new(pEntity);
}
void InstallHooks() {
    // Render hooks
    CHook::Redirect("_Z13Render2dStuffv", &Render2dStuff);
    CHook::Redirect("_Z13RenderEffectsv", &RenderEffects);
    CHook::InlineHook("_Z14AND_TouchEventiiii", &AND_TouchEvent_hook, &AND_TouchEvent);
    // HUD and radar hooks
    CHook::Redirect("_ZN11CHudColours12GetIntColourEh", &CHudColours__GetIntColour); // dangerous
    CHook::Redirect("_ZN6CRadar19GetRadarTraceColourEjhh",
                    &CRadar__GetRadarTraceColor); // dangerous
    CHook::InlineHook("_ZN6CRadar12SetCoordBlipE9eBlipType7CVectorj12eBlipDisplayPc",
                      &CRadar__SetCoordBlip_hook, &CRadar__SetCoordBlip);
    CHook::InlineHook("_ZN6CRadar20DrawRadarGangOverlayEb", &CRadar_DrawRadarGangOverlay_hook,
                      &CRadar_DrawRadarGangOverlay);

    CHook::InlineHook("_ZN8CCoronas6RenderEv", &CCoronas__Render_hook, &CCoronas__Render_orig);

    // Hide GTA clock text drawn above money in the HUD.
#if VER_x32
    CHook::InlineHook(g_libGTASA + 0x005AA200 + 1, &CFont_PrintString_hook, &CFont_PrintString);
#else
    CHook::InlineHook(g_libGTASA + 0x6CDEB0, &CFont_PrintString_hook, &CFont_PrintString);
    CHook::InlineHook(g_libGTASA + 0x35B298, &SelectScreen_AddItem_hook,
                      &SelectScreen_AddItem_orig);
    CHook::InlineHook(g_libGTASA + 0x5140B8, &CMenuManager_DrawFrontEnd_hook, &CMenuManager_DrawFrontEnd);
    CHook::InlineHook(g_libGTASA + 0x516B84, &CMenuManager_Process_hook, &CMenuManager_Process);
    CHook::InlineHook(g_libGTASA + 0x355DFC, &MobileMenu_Render_hook, &MobileMenu_Render);
    CHook::InlineHook(g_libGTASA + 0x356A7C, &MobileMenu_Update_hook, &MobileMenu_Update);
#endif

    // Texture hooks
    CHook::Redirect("_Z10GetTexturePKc", &CUtil::GetTexture);

    // Menu hooks
    CHook::InlineHook("_ZN14MainMenuScreen6OnExitEv", &MainMenuScreen__OnExit_hook,
                      &MainMenuScreen__OnExit);

    CHook::InlineHook("_ZN7CEntity6RenderEv", &CEntity_Render_hook_new, &CEntity_Render_orig_new);

    // Weapon and combat hooks
    CHook::InlineHook("_ZN17CTaskSimpleUseGun17RemoveStanceAnimsEP4CPedf", &CTaskSimpleUseGun__RemoveStanceAnims_hook, &CTaskSimpleUseGun__RemoveStanceAnims);

    CHook::InlineHook("_ZN7CWeapon14FireInstantHitEP7CEntityP7CVectorS3_S1_S3_S3_bb", &CWeapon__FireInstantHit_hook, &CWeapon__FireInstantHit);
    CHook::InlineHook("_ZN7CWeapon10FireSniperEP4CPedP7CEntityP7CVector", &CWeapon__FireSniper_hook, &CWeapon__FireSniper);
    CHook::InlineHook("_ZN6CWorld18ProcessLineOfSightERK7CVectorS2_R9CColPointRP7CEntitybbbbbbbb", &CWorld__ProcessLineOfSight_hook, &CWorld__ProcessLineOfSight);
    CHook::InlineHook("_ZN28CPedDamageResponseCalculator21ComputeDamageResponseEP4CPedR18CPedDamageResponseb", &CPedDamageResponseCalculator__ComputeDamageResponse_hook, &CPedDamageResponseCalculator__ComputeDamageResponse);
    CHook::InlineHook("_ZN7CWeapon18ProcessLineOfSightERK7CVectorS2_R9CColPointRP7CEntity11eWeaponTypeS6_bbbbbbb", &CWeapon__ProcessLineOfSight_hook, &CWeapon__ProcessLineOfSight);
    CHook::InlineHook("_ZN11CBulletInfo9AddBulletEP7CEntity11eWeaponType7CVectorS3_", &CBulletInfo_AddBullet_hook, &CBulletInfo_AddBullet);

    CHook::InlineHook("_ZN11CFileLoader18LoadObjectInstanceEPKc", &CFileLoader__LoadObjectInstance_hook, &CFileLoader__LoadObjectInstance);

    CHook::InlineHook("_ZN6CRadar9ClearBlipEi", &CRadar_ClearBlip_hook, &CRadar_ClearBlip);

    CHook::InlineHook(
            "_ZN10CCollision19ProcessVerticalLineERK8CColLineRK7CMatrixR9CColModelR9CColPointRfbbP15CStoredCollPoly",
            &CCollision__ProcessVerticalLine_hook, &CCollision__ProcessVerticalLine);

    CHook::InlineHook("_ZN19CUpsideDownCarCheck15IsCarUpsideDownEPK8CVehicle",
                      &CUpsideDownCarCheck__IsCarUpsideDown_hook,
                      &CUpsideDownCarCheck__IsCarUpsideDown);

    CHook::InlineHook("_ZN16CTaskSimpleGetUp10ProcessPedEP4CPed",
                      &CTaskSimpleGetUp__ProcessPed_hook,
                      &CTaskSimpleGetUp__ProcessPed); // CTaskSimpleGetUp::ProcessPed
    CHook::InlineHook("_ZN7CObject6RenderEv", &CObject_Render_hook, &CObject_Render);

    CHook::Redirect("_Z19PlayerIsEnteringCarv", &PlayerIsEnteringCar);
    if (*(uint8_t *) (g_libGTASA + (VER_x32 ? 0x6B8B9C : 0x896135))) {
        CHook::Redirect("_ZNK14TextureListing11GetMipCountEv", &getmip);
    }

    if (!eglGetProcAddress("glAlphaFuncQCOM")) {

        if (eglGetProcAddress("glAlphaFunc")) {
            *((void **) (g_libGTASA +
                         (VER_x32 ? 0x6BCBF8 : 0x89A1B0))) = (void *) eglGetProcAddress(
                    "glAlphaFunc");
        } else {
            CHook::Redirect("_Z25RQ_Command_rqSetAlphaTestRPc", &RQCommand_rqSetAlphaTest);
        }
    }

    CHook::Redirect("_ZN4CHID12GetInputTypeEv", &GetInputType);

#if VER_x32
    CHook::InlineHook("_ZN14CAnimBlendNode12FindKeyFrameEf", &CAnimBlendNode__FindKeyFrame_hook, &CAnimBlendNode__FindKeyFrame);
    CHook::InlineHook("_ZN15CClumpModelInfo14GetFrameFromIdEP7RpClumpi", &CClumpModelInfo_GetFrameFromId_hook, &CClumpModelInfo_GetFrameFromId);
#endif

    CHook::InlineHook("_ZN13FxEmitterBP_c6RenderEP8RwCamerajfh", &FxEmitterBP_c__Render_hook,
                      &FxEmitterBP_c__Render);
    CHook::InlineHook("_Z23RwResourcesFreeResEntryP10RwResEntry", &RwResourcesFreeResEntry_hook,
                      &RwResourcesFreeResEntry);

    //CHook::InlineHook("_ZN9CRenderer24RenderEverythingBarRoadsEv", &CRenderer_RenderEverythingBarRoads_hook, &CRenderer_RenderEverythingBarRoads);

    ms_fAspectRatio = (float*)(g_libGTASA+(VER_x32 ? 0xA26A90:0xCC7F00));
    CHook::InlineHook("_ZN4CHud14DrawCrossHairsEv", &DrawCrosshair_hook, &DrawCrosshair);

    //CHook::InlineHook("_ZN26CAEGlobalWeaponAudioEntity21ServiceAmbientGunFireEv", &TaskEnterVehicleHook, &TaskEnterVehicle);
#if VER_x32
    CHook::UnFuck(g_libGTASA + 0x4DD9E8);
    *(float*)(g_libGTASA + 0x4DD9E8) = 0.015f;
#else
    CHook::Write(g_libGTASA + 0x5DF790, 0x90000AA9);
    CHook::Write(g_libGTASA + 0x5DF794, 0xBD48D521);
#endif

    CHook::InlineHook("_ZN5CDraw6SetFOVEfb", &CDraw__SetFOV_hook, &CDraw__SetFOV);

    CHook::InlineHook("_ZN10CStreaming5Init2Ev", &CStreaming__Init2_hook, &CStreaming__Init2);

/*#if VER_x32
    CHook::InstallPLT( g_libGTASA + 0x66F3D4, &mpg123_param_hook, &mpg123_param);
#else
    CHook::Write(g_libGTASA + 0x339134, 0x52846C02);
    CHook::Write(g_libGTASA + 0x339404, 0x52846C02);
#endif*/

    HookCPad();
}