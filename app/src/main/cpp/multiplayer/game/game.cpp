#include "../main.h"
#include "game.h"
#include "../vendor/armhook/patch.h"
#include "Streaming.h"
#include "MemoryMgr.h"
#include "CFileMgr.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "Scene.h"
#include "game/Samsung/NavigationPauseFix.h"
#include "TxdStore.h"
#include "VisibilityPlugins.h"
#include "net/netgame.h"
#include "CrossHair.h"
#include "Pickups.h"
#include "game/Models/ModelInfo.h"
#include "game/Collision/Collision.h"
#include "World.h"
#include "util.h"
#include "game/patches.h"
#include "COcclusion.h"
#include "CGPS.hpp"
#include "java/Hud.h"

extern GPS* pGPS;
extern CNetGame* pNetGame;

void ApplySAMPPatchesInGame();
void InitScripting();

constexpr uint32_t kRadarColorFixedRed = 1005;

bool bUsedPlayerSlots[PLAYER_PED_SLOTS];

uint16_t *szGameTextMessage;

bool CGame::bIsGameExiting = false;

bool CGame::IsValidPlayerSlot(uint8_t slot)
{
    return slot >= 2 && slot < PLAYER_PED_SLOTS;
}

int CGame::FindFirstFreePlayerPedSlotSafe()
{
    for (uint8_t x = 2; x < PLAYER_PED_SLOTS; ++x) {
        if (!bUsedPlayerSlots[x]) {
            FLog("Found free slot: %d", x);
            return (int)x;
        }
    }
    FLog("No free slot found!");
    return -1;
}

CGame::CGame()
{
    m_pGamePlayer = nullptr;
    m_bCheckpointsEnabled = false;
    m_bRaceCheckpointsEnabled = false;
    m_dwRaceCheckpointHandle = 0;

    m_bClockEnabled = false;
    m_bInputEnable = true;

    memset(bUsedPlayerSlots, 0, sizeof(bUsedPlayerSlots));
    memset(m_bPreloadedVehicleModels, 0, sizeof(m_bPreloadedVehicleModels));

    // Инициализация всех указателей
    m_dwCheckpointMarker = 0;
    m_dwRaceCheckpointMarker = 0;
    m_vecCheckpointPos = CVector(0, 0, 0);
    m_vecCheckpointExtent = CVector(0, 0, 0);
    m_vecRaceCheckpointPos = CVector(0, 0, 0);
    m_vecRaceCheckpointNextPos = CVector(0, 0, 0);
    m_byteRaceType = 0;
    m_fRaceCheckpointRadius = 0.0f;
}

CGame::~CGame()
{
    // Очистка памяти
    if (szGameTextMessage) {
        delete[] szGameTextMessage;
        szGameTextMessage = nullptr;
    }

    if (m_pGamePlayer) {
        delete m_pGamePlayer;
        m_pGamePlayer = nullptr;
    }
}

void ApplyGlobalPatches();
void InstallHooks();
void CGame::StartGame()
{
    FLog("Starting game..");

    InstallHooks();
    ApplyGlobalPatches();

    GameAimSyncInit();
    InitScripting();
}

void InstallSAMPHooks();
void InstallWidgetHooks();

void CGame::Initialize()
{
    FLog("CGame initializing..");

    ApplySAMPPatchesInGame();
    GameResetRadarColors();
    szGameTextMessage = new uint16_t[1076];
}

// 0.3.7
void CGame::SetMaxStats()
{
    CHook::CallFunction<void>("_ZN6CCheat18VehicleSkillsCheatEv");
    CHook::CallFunction<void>("_ZN6CCheat17WeaponSkillsCheatEv");

    // CStats::SetStatValue nop
    CHook::RET("_ZN6CStats12SetStatValueEtf");
}

// 0.3.7
void CGame::ToggleThePassingOfTime(bool bOnOff)
{
    if (bOnOff)
    {
        CHook::WriteMemory(g_libGTASA + 0x3E33C8, (uintptr_t)"\xD0\xB5", 2);
        this->m_bClockEnabled = true;
    }
    else
    {
        CHook::RET(g_libGTASA + 0x3E33C8);
        this->m_bClockEnabled = false;
    }
}

// 0.3.7
void CGame::EnableClock(bool bEnable)
{
    /*char byteClockData[] = { '%', '0', '2', 'd', ':', '%', '0', '2', 'd', 0 };
    CHook::UnFuck(g_libGTASA + 0x2BD618);

    if (bEnable)
    {
       ToggleThePassingOfTime(true);
       memcpy((void*)(g_libGTASA + 0x2BD618), byteClockData, 10);
    }
    else
    {
       ToggleThePassingOfTime(false);
       memset((void*)(g_libGTASA + 0x2BD618), 0, 10);
    }*/
}

// 0.3.7
void CGame::EnableZoneNames(bool bEnable)
{
    ScriptCommand(&enable_zone_names, bEnable);
}

// 0.3.7
void CGame::SetWorldTime(int iHour, int iMinute)
{
    if (iHour < 0 || iHour > 23 || iMinute < 0 || iMinute > 59) {
        FLog("Invalid time: %02d:%02d", iHour, iMinute);
        return;
    }

    *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953143 : 0xBBBC1B)) = (uint8_t)iMinute;
    *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953142 : 0xBBBC1A)) = (uint8_t)iHour;
    ScriptCommand(&set_current_time, iHour, iMinute);
}

// 0.3.7
void CGame::GetWorldTime(int *iHour, int *iMinute)
{
    if (iHour) *iHour = *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953142 : 0xBBBC1A));
    if (iMinute) *iMinute = *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x00953143 : 0xBBBC1B));
}

// 0.3.7
void CGame::SetWorldWeather(int byteWeatherID)
{
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005CDF88 + 1 : 0x6F24E8), byteWeatherID);

    if(!m_bClockEnabled)
    {
        *(uint16_t*)(g_libGTASA + (VER_x32 ? 0x00A7D136 : 0xD216F2)) = byteWeatherID;
        *(uint16_t*)(g_libGTASA + (VER_x32 ? 0x00A7D134 : 0xD216F0)) = byteWeatherID;
    }
}

// 0.3.7
void CGame::DisplayHUD(bool bDisp)
{
    if (bDisp)
    {
        *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x819D88 : 0x9FF3A8)) = 1;
        *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x991FD8 : 0xC20DFC)) = 0;
    }
    else
    {
        *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x819D88 : 0x9FF3A8)) = 0;
        *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x991FD8 : 0xC20DFC)) = 1;
    }
}

// 0.3.7
uint8_t CGame::GetActiveInterior()
{
    return CGame::currArea;
}

const char* CGame::GetDataDirectory()
{
    return ""; // StorageRootBuffer
}

// 0.3.7
void CGame::UpdateCheckpoints()
{
    if (m_bCheckpointsEnabled)
    {
        CPlayerPed* pPlayerPed = this->FindPlayerPed();
        if (pPlayerPed)
        {
            ScriptCommand(&is_actor_near_point_3d, pPlayerPed->m_dwGTAId,
                          m_vecCheckpointPos.x, m_vecCheckpointPos.y, m_vecCheckpointPos.z,
                          m_vecCheckpointExtent.x, m_vecCheckpointExtent.y, m_vecCheckpointExtent.z, 1);

            if (!m_dwCheckpointMarker)
            {
                m_dwCheckpointMarker = CreateRadarMarkerIcon(0, m_vecCheckpointPos.x,
                                                             m_vecCheckpointPos.y, m_vecCheckpointPos.z, kRadarColorFixedRed, 2);
            }
        }
    }
    else if (m_dwCheckpointMarker)
    {
        DisableMarker(m_dwCheckpointMarker);
        m_dwCheckpointMarker = 0;
    }

    if (m_bRaceCheckpointsEnabled)
    {
        CPlayerPed* pPlayerPed = this->FindPlayerPed();
        if (pPlayerPed)
        {
            if (!m_dwRaceCheckpointMarker)
            {
                m_dwRaceCheckpointMarker = CreateRadarMarkerIcon(0, m_vecRaceCheckpointPos.x,
                                                                 m_vecRaceCheckpointPos.y, m_vecRaceCheckpointPos.z, kRadarColorFixedRed, 2);
            }
        }
    }
    else if (m_dwRaceCheckpointMarker)
    {
        DisableMarker(m_dwRaceCheckpointMarker);
        DisableRaceCheckpoint();
        m_dwRaceCheckpointMarker = 0;
    }
}

// 0.3.7
uint8_t CGame::GetPedSlotsUsed()
{
    uint8_t count = 0;
    for (int i = 2; i < PLAYER_PED_SLOTS; i++)
    {
        if (bUsedPlayerSlots[i])
            count++;
    }

    return count;
}

void CGame::PlaySound(int iSound, float fX, float fY, float fZ)
{
    ScriptCommand(&play_sound, fX, fY, fZ, iSound);
}

// 0.3.7
void CGame::RefreshStreamingAt(float x, float y)
{
    ScriptCommand(&refresh_streaming_at, x, y);
}

// 0.3.7
void CGame::DisableTrainTraffic()
{
    ScriptCommand(&enable_train_traffic, 0);
}

// 0.3.7
void CGame::UpdateGlobalTimer(uint32_t dwTimer)
{
    if (!m_bClockEnabled)
    {
        *(uint32_t*)(g_libGTASA + 0x96B4D8) = dwTimer & 0x3FFFFFFF;
    }
}

// 0.3.7
void CGame::SetGravity(float fGravity)
{
#if VER_x32
    CHook::UnFuck(g_libGTASA + (VER_2_1 ? 0x003FE810 : 0x3A0B64));
    *(float*)(g_libGTASA + (VER_2_1 ? 0x003FE810 : 0x3A0B64)) = fGravity;
#endif
}

bool CGame::IsGamePaused()
{
    return *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x96B514 : 0xBDC594));
}

bool CGame::IsGameLoaded()
{
    return true;
}

void CGame::DrawGangZone(float fPos[], uint32_t dwColor, uint32_t dwUnk)
{
    // CRadar::DrawAreaOnRadar
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00443C60 + 1 : 0x528EC4), fPos, &dwColor, dwUnk);
}

// 0.3.7
uint32_t CGame::CreatePickup(int iModel, int iType, float x, float y, float z, int *pdwIndex)
{
    uintptr hnd;

    auto dwModelArray = CModelInfo::ms_modelInfoPtrs;
    if(dwModelArray[iModel] == nullptr)
        iModel = 18631; // вопросик

    ScriptCommand(&create_pickup, iModel, iType, x, y, z, &hnd);

    int lol = 32 * (uint16_t)hnd;
    if(lol) lol /= 32;
    if(pdwIndex) *pdwIndex = lol;

    return hnd;
}

// 0.3.7
bool CGame::IsModelLoaded(int iModel)
{
    if (iModel > 20000 || iModel < 0) {
        return true;
    }
    else {
        return ScriptCommand(&is_model_available, iModel);
    }
}

// 0.3.7
void CGame::RequestModel(uint16_t iModelId, uint8_t iLoadingStream)
{
    ScriptCommand(&request_model, iModelId);
}

// 0.3.7
void CGame::LoadRequestedModels()
{
    ScriptCommand(&load_requested_models);
}

// 0.3.7
void CGame::RemoveModel(int iModel, bool bFromStreaming)
{
    if (iModel >= 0 && iModel < 20000)
    {
        if (bFromStreaming)
        {
            if(ScriptCommand(&is_model_available, iModel))
                // CStreaming::RemoveModel x64 0000000000391FF0 x32 002D0128
                ((void(*)(int))(g_libGTASA + (VER_x32 ? 0x2D0128 + 1 : 0x391FF0)))(iModel);
        }
        else
        {
            if (ScriptCommand(&is_model_available, iModel))
                ScriptCommand(&release_model, iModel);
        }
    }
}

// 0.3.7 ( 2     0.3DL)
CObject* CGame::NewObject(int iModel, CVector vecPos, CVector vecRot, float fDrawDistance)
{
    CObject *pObjectNew = new CObject(iModel, vecPos, vecRot, fDrawDistance, 0);
    return pObjectNew;
}

// 0.3.7 (   bIsNPC)
CPlayerPed* CGame::NewPlayer(int iSkin, float fX, float fY, float fZ, float fRotation, bool unk, bool bIsNPC)
{
    int slot = FindFirstFreePlayerPedSlotSafe();
    if (slot < 2 || slot >= PLAYER_PED_SLOTS) {
        FLog("Invalid player slot: %d", slot);
        return nullptr;
    }

    uint8_t bytePedSlot = (uint8_t)slot;
    auto pPed = new CPlayerPed(bytePedSlot, iSkin, fX, fY, fZ, fRotation);
    if (!pPed || !pPed->m_pPed) {
        FLog("Failed to create player ped at slot %d", bytePedSlot);
        if (pPed) {
            delete pPed;
        }
        return nullptr;
    }

    bUsedPlayerSlots[bytePedSlot] = true;
    FLog("Created player ped at slot %d", bytePedSlot);

    return pPed;
}

// 0.3.7
bool CGame::RemovePlayer(CPlayerPed* pPed)
{
    if (!pPed) return false;

    uint8_t slot = pPed->m_bytePlayerNumber;
    if (IsValidPlayerSlot(slot)) {
        bUsedPlayerSlots[slot] = false;
    }

    delete pPed;
    return true;
}

// 0.3.7
void CGame::DisableMarker(uint32_t dwMarker)
{
    ScriptCommand(&disable_marker, dwMarker);
}

// 0.3.7
uint32_t CGame::CreateRadarMarkerIcon(uint8_t byteType, float fPosX, float fPosY, float fPosZ, uint32_t dwColor, uint8_t byteStyle)
{
    uintptr dwMarkerID = 0;

    if(byteStyle == 1)
        ScriptCommand(&create_marker_icon, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);
    else if(byteStyle == 2)
        ScriptCommand(&create_radar_marker_icon, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);
    else if(byteStyle == 3)
        ScriptCommand(&create_icon_marker_sphere, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);
    else
        ScriptCommand(&create_radar_marker_without_sphere, fPosX, fPosY, fPosZ, byteType, &dwMarkerID);

    if(byteType == 0)
    {
        if(dwColor >= 1004)
        {
            ScriptCommand(&set_marker_color, dwMarkerID, dwColor);
            ScriptCommand(&show_on_radar, dwMarkerID, 3);
        }
        else
        {
            ScriptCommand(&set_marker_color, dwMarkerID, dwColor);
            ScriptCommand(&show_on_radar, dwMarkerID, 2);
        }
    }

    return dwMarkerID;
}

// 0.3.7
bool CGame::IsAnimationLoaded(const char* szAnimLib)
{
    return ScriptCommand(&is_animation_loaded, szAnimLib);
}

// 0.3.7
void CGame::RequestAnimation(const char* szAnimLib)
{
    ScriptCommand(&request_animation, szAnimLib);
}

// 0.3.7
float CGame::FindGroundZForCoord(float fX, float fY, float fZ)
{
    float fGroundZ;
    ScriptCommand(&get_ground_z, fX, fY, fZ, &fGroundZ);
    return fGroundZ;
}

// 0.3.7
void CGame::DisableAutoAim()
{
    CHook::RET(g_libGTASA + 0x4C6CF4); // CPlayerPed::FindWeaponLockOnTarget
    CHook::RET(g_libGTASA + 0x4C7CDC); // CPlayerPed::FindNextWeaponLockOnTarget

    // CPed::SetWeaponLockOnTarget
    CHook::RET(g_libGTASA + 0x4A82D4/*0x438DB4*/);
}

// 0.3.7
void CGame::EnabledAutoAim()
{
    CHook::RET(g_libGTASA + 0x4C6CF4); // CPlayerPed::FindWeaponLockOnTarget
    CHook::RET(g_libGTASA + 0x4C7CDC); // CPlayerPed::FindNextWeaponLockOnTarget
}

// 0.3.7
CVehicle* CGame::NewVehicle(int iVehicleType, float fX, float fY, float fZ, float fRotation, bool bAddSiren)
{
    if (iVehicleType < 400 || iVehicleType >= 612) {
        FLog("Invalid vehicle type: %d", iVehicleType);
        return nullptr;
    }

    bool bPreloaded = true;
    int modelIndex = iVehicleType - 400;
    if (modelIndex >= 0 && modelIndex < MAX_VEHICLE_MODELS) {
        if (m_bPreloadedVehicleModels[modelIndex] == true) {
            bPreloaded = true;
        }
    }

    CVehicle* pNewVehicle = new CVehicle(iVehicleType, fX, fY, fZ, fRotation, bPreloaded, bAddSiren);

    return pNewVehicle;
}

// 0.3.7
void CGame::SetCheckpointInformation(CVector* vecPos, CVector* vecSize)
{
    if (!vecPos || !vecSize) return;

    m_vecCheckpointPos.x = vecPos->x;
    m_vecCheckpointPos.y = vecPos->y;
    m_vecCheckpointPos.z = vecPos->z;

    m_vecCheckpointExtent.x = vecSize->x;
    m_vecCheckpointExtent.y = vecSize->y;
    m_vecCheckpointExtent.z = vecSize->z;

    if (m_dwCheckpointMarker)
    {
        DisableMarker(m_dwCheckpointMarker);
        m_dwCheckpointMarker = 0;
    }

    m_dwCheckpointMarker = CreateRadarMarkerIcon(0,
                                                 m_vecCheckpointPos.x,
                                                 m_vecCheckpointPos.y,
                                                 m_vecCheckpointPos.z,
                                                 kRadarColorFixedRed, 2);
}

// 0.3.7
void CGame::SetRaceCheckpointInformation(uint8_t byteType, CVector* vecPos, CVector* vecNextPos, float fRadius)
{
    if (!vecPos || !vecNextPos) return;

    m_vecRaceCheckpointPos.x = vecPos->x;
    m_vecRaceCheckpointPos.y = vecPos->y;
    m_vecRaceCheckpointPos.z = vecPos->z;

    m_vecRaceCheckpointNextPos.x = vecNextPos->x;
    m_vecRaceCheckpointNextPos.y = vecNextPos->y;
    m_vecRaceCheckpointNextPos.z = vecNextPos->z;

    m_byteRaceType = byteType;
    m_fRaceCheckpointRadius = fRadius;

    if (m_dwRaceCheckpointMarker)
    {
        DisableMarker(m_dwRaceCheckpointMarker);
    }

    m_dwRaceCheckpointMarker = CreateRadarMarkerIcon(0,
                                                     m_vecRaceCheckpointPos.x,
                                                     m_vecRaceCheckpointPos.y,
                                                     m_vecRaceCheckpointPos.z,
                                                     kRadarColorFixedRed,
                                                     2);

    MakeRaceCheckpoint();
}

// 0.3.7
void CGame::MakeRaceCheckpoint()
{
    if(m_bRaceCheckpointsEnabled)
    {
        DisableRaceCheckpoint();
        GPS::Set(m_vecRaceCheckpointPos, false);
    }

    ScriptCommand(&create_racing_checkpoint, (int)m_byteRaceType,
                  m_vecRaceCheckpointPos.x, m_vecRaceCheckpointPos.y, m_vecRaceCheckpointPos.z,
                  m_vecRaceCheckpointNextPos.x, m_vecRaceCheckpointNextPos.y, m_vecRaceCheckpointNextPos.z,
                  m_fRaceCheckpointRadius, &m_dwRaceCheckpointHandle);

    GPS::Set(m_vecRaceCheckpointPos, true);

    m_bRaceCheckpointsEnabled = true;
}

// 0.3.7
void CGame::DisableRaceCheckpoint()
{
    if (m_dwRaceCheckpointHandle)
    {
        ScriptCommand(&destroy_racing_checkpoint, m_dwRaceCheckpointHandle);
        m_dwRaceCheckpointHandle = 0;
    }

    m_bRaceCheckpointsEnabled = false;
}

void CGame::SetWantedLevel(uint8_t level)
{
    //CHook::WriteMemory(g_libGTASA+0x2BDF6E, (const void*)&level, 1);
    CHud::iWantedLevel = level;
    CHud::UpdateWanted();
}

void CGame::EnableStuntBonus(bool bEnable)
{
    //CHook::UnFuck(0x7BE2A8);
    //*(int*)(g_libGTASA+0x7BE2A8) = (int)bEnable;
}

// 0.3.7
void CGame::DisplayGameText(const char* szStr, int iTime, int iSize)
{
    if (!szStr) return;

    ScriptCommand(&text_clear_all);
    CFont::AsciiToGxtChar(szStr, szGameTextMessage);

    // CMessages::AddBigMesssage
    (( void (*)(uint16_t*, int, int))(g_libGTASA + (VER_x32 ? 0x0054C62C + 1 : 0x66C150)))(szGameTextMessage, iTime, iSize);
}

// 0.3.7
void CGame::AddToLocalMoney(int iAmmount)
{
    ScriptCommand(&add_to_player_money, 0, iAmmount);
    CHud::iLocalMoney = iAmmount;
    CHud::UpdateMoney();
}

// 0.3.7
void CGame::ResetLocalMoney()
{
    int iMoney = GetLocalMoney();
    if (!iMoney) return;

    AddToLocalMoney(-iMoney);
}

// 0.3.7
int CGame::GetLocalMoney()
{
    return FindPlayerInfo().Score;
}

uint8_t CGame::GetWantedLevel()
{
    return *(uint8_t*)(g_libGTASA + (VER_x32 ? 0x002BDFDC : 0x0037E160));
}

// 0.3.7
void CGame::DisableEnterExits()
{
#if VER_x32
    uintptr_t addr = *(uintptr_t*)(g_libGTASA + (VER_2_1 ? 0x007A1E20 : 0x700120));
    if (!addr) return;

    int count = *(uint32_t*)(addr+8);

    addr = *(uintptr_t*)addr;

    for(int i=0; i<count; i++)
    {
        *(uint16_t*)(addr+0x30) = 0;
        addr += 0x3C;
    }
#endif
}

void CGame::ToggleCJWalk(bool bUseCJWalk)
{
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x004C5F6A : 0x5C3970), 2);
}

void CGame::InitialiseOnceBeforeRW() {
    CHook::CallFunction<void>("_ZN14MobileSettings10InitializeEv");
    CHook::CallFunction<void>("_ZN13CLocalisation10InitialiseEv");
    CFileMgr::Initialise();
    CdStreamInit(TOTAL_IMG_ARCHIVES);
    CHook::CallFunction<void>("_ZN4CPad10InitialiseEv");
}

void CameraSize(RwCamera* camera, RwRect* rect, RwReal viewWindow, RwReal aspectRatio) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D32AC + 1 : 0x6F7F84), camera, rect, viewWindow, aspectRatio);
}

void CameraDestroy(RwCamera* camera) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D33A4 + 1 : 0x6F80C0), camera);
}

void LightsCreate(RpWorld* world) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0046FC08 + 1 : 0x55BDCC), world);
}

void InitGui();

bool CGame::InitialiseRenderWare() {
    FLog("InitialiseRenderWare ..");

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

    CTxdStore::Initialise();
    CVisibilityPlugins::Initialise();

    TextureDatabaseRuntime::Load("samp", false, TextureDatabaseFormat::DF_DXT);
    TextureDatabaseRuntime::Load("mobile", true, TextureDatabaseFormat::DF_DXT);
    TextureDatabaseRuntime::Load("txd", false, TextureDatabaseFormat::DF_DXT);
    TextureDatabaseRuntime::Load("gui", false, TextureDatabaseFormat::DF_DXT);
    TextureDatabaseRuntime::Load("gta3", false, TextureDatabaseFormat::DF_DXT);
    TextureDatabaseRuntime::Load("gta_int", false, TextureDatabaseFormat::DF_DXT);
    TextureDatabaseRuntime::Load("menu", false, TextureDatabaseFormat::DF_DXT);

    const auto camera = RwCameraCreate();
    if (!camera) {
        return false;
    }

    const auto frame = RwFrameCreate();
    rwObjectHasFrameSetFrame(&camera->object.object, frame);
    camera->frameBuffer = RwRasterCreate(RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0, rwRASTERTYPECAMERA);
    camera->zBuffer = RwRasterCreate(RsGlobal->maximumWidth, RsGlobal->maximumHeight, 0, rwRASTERTYPEZBUFFER);
    if (!camera->object.object.parent) {
        CameraDestroy(camera);
        return false;
    }
    Scene.m_pRwCamera = camera;
    TheCamera.Init();
    TheCamera.SetRwCamera(Scene.m_pRwCamera);
    RwCameraSetFarClipPlane(Scene.m_pRwCamera, 2000.0f);
    RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.9f);
    CameraSize(Scene.m_pRwCamera, nullptr, 0.7f, 4.0f / 3.0f);

    RwBBox bb;
    bb.sup = { 10'000.0f,  10'000.0f,  10'000.0f};
    bb.inf = {-10'000.0f, -10'000.0f, -10'000.0f};

    if (Scene.m_pRpWorld = RpWorldCreate(&bb); !Scene.m_pRpWorld) {
        CameraDestroy(Scene.m_pRwCamera);
        Scene.m_pRwCamera = nullptr;
        return false;
    }
    RpWorldAddCamera(Scene.m_pRpWorld, Scene.m_pRwCamera);
    LightsCreate(Scene.m_pRpWorld);
    CFont::Initialise();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0046FF38 + 1 : 0x55C1C8)); // CHud::Initialise();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B1188 + 1 : 0x6D5970)); // CPlayerSkin::Initialise();
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005B28D4 + 1 : 0x6D6E30)); // CPostEffects::Initialise();
    CGame::m_pWorkingMatrix1 = RwMatrixCreate();
    CGame::m_pWorkingMatrix2 = RwMatrixCreate();

    InitGui();

    return true;
}

void CGame::PostToMainThread(std::function<void()> task)
{
    if (!task) return;

    std::lock_guard<std::mutex> lock(mtx);
    tasks.push(std::move(task));
}

void CGame::ProcessMainThreadTasks()
{
    if (tasks.empty())
        return;

    try {
        std::function<void()> task;
        {
            std::lock_guard<std::mutex> lock(mtx);
            task = std::move(tasks.front());
            tasks.pop();
        }
        if (task) {
            task();
        }
    } catch (const std::exception& e) {
        FLog("Exception in ProcessMainThreadTasks: %s", e.what());
    } catch (...) {
        FLog("Unknown exception in ProcessMainThreadTasks");
    }
}

extern CGame* pGame;
extern CNetGame* pNetGame;
extern UI *pUI;

void MainLoop();

#include "../util/CSnapShots.h"
#include "../util/CSnapShotWrapper.h"
void CGame::Process() {
    if(bIsGameExiting) return;

    static bool once = false;
    if (!once)
    {
        // FLog("lj111111111111111111111111119000000000000000000000000000000000000000000okp");
        CSnapShots::Initialise();
        once = true;
    } else {
        //FLog("poodhujjjjjjjjjjjjjjjjjjjj999999999999999999999999999999jjjjj");
        CSnapShotWrapper::Process();
    }

    try {
        MainLoop();

        // Prevent occlusion culling from hiding server-created objects.
        if (COcclusion::NumOccludersOnMap > 0) {
            COcclusion::NumOccludersOnMap = 0;
        }

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

        ProcessMainThreadTasks();

        uint32_t CurrentTimeInCycles;
        uint32_t v1;
        uint32_t v2;
        uint32_t v3;

        ((void(*)())(g_libGTASA + (VER_x32 ? 0x003F8B50 + 1 : 0x4DB464)))(); // CPad::UpdatePads()
        ((void(*)())(g_libGTASA + (VER_x32 ? 0x002B03F8 + 1 : 0x36F374)))(); // CTouchInterface::Clear()
        ((void(*)())(g_libGTASA + (VER_x32 ? 0x0028C178 + 1 : 0x3467BC)))(); // CHID::Update()

        CurrentTimeInCycles = CTimer::GetCurrentTimeInCycles();
        v1 = CurrentTimeInCycles / CTimer::GetCyclesPerMillisecond();

        CStreaming::Update();

        v2 = CTimer::GetCurrentTimeInCycles();
        v3 = v2 / CTimer::GetCyclesPerMillisecond();

        if ( !(CTimer::m_CodePause << 0x18) )
        {
            auto gMobileMenu = (uintptr_t *) (g_libGTASA + (VER_x32 ? 0x006E0074 : 0x8BE780));
            ((void(*)(uintptr_t*))(g_libGTASA + (VER_x32 ? 0x0029A730 + 1 : 0x356A7C)))(gMobileMenu); // MobileMenu::Update
        }

        CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
        TheCamera.m_fLODDistMultiplier = 2.0f;
        TheCamera.GenerationDistMultiplier = 2.0f;

        *(int32_t*)(g_libGTASA + (VER_x32 ? 0x00A7D22C: 0xD217F8)) = 0; // CWindModifiers::Number

        if ( !CTimer::m_CodePause && !CTimer::m_UserPause )
        {
            CSprite2d::SetRecipNearClip();
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005C89F8 + 1 : 0x6ECF00)))(); // CSprite2d::InitPerFrame();
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A8A74 + 1 : 0x6CC898)))(); // CFont::InitPerFrame()

            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005CC2E8 + 1 : 0x6F0BD8)))(); // CWeather::Update()
            ((void(*)())(g_libGTASA + (VER_x32 ? 0x0032AED8 + 1 : 0x3F3AD8)))(); // CTheScripts::Process()

            CHook::CallFunction<void>(g_libGTASA+(VER_x32?0x57D098+1:0x6A0A14));// CTrain::UpdateTrains();
            ((void(*)())(g_libGTASA + (VER_x32 ? 0x005BE838 + 1 : 0x6E2F08)))(); // CSkidmarks::Update();
            ((void(*)())(g_libGTASA + (VER_x32 ? 0x005AB4C8 + 1 : 0x6D032C)))(); // CGlass::Update()

            auto gFireManager = (uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00958800 : 0xBC12D8));
            ((void (*)(uintptr_t *)) (g_libGTASA + (VER_x32 ? 0x003F1628 + 1 : 0x4D361C)))(gFireManager); // CFireManager::Update

            ((void(*)(bool))(g_libGTASA + (VER_x32 ? 0x004CC380 + 1 : 0x5CB5E0)))(false); // CPopulation::Update

            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005DB8E8 + 1 : 0x700AF4)))(); // CWeapon::UpdateWeapons()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A6720 + 1 : 0x6CA130)))(); // CMovingThings::Update();
            ((void(*)())(g_libGTASA + (VER_x32 ? 0x005CBB20 + 1 : 0x6F04CC)))(); // CWaterCannons::Update()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x00427744 + 1 : 0x50BE40)))(); // CWorld::Process()

            if ( !CTimer::bSkipProcessThisFrame )
            {
                CPickups::Update();
                CHook::CallFunction<void>(g_libGTASA+(VER_x32?0x30E760+1:0x3D4134)); //CGarages::Update();
                CHook::CallFunction<void>(g_libGTASA+(VER_x32?0x3616C4+1:0x4304D0)); // CStuntJumpManager::Update();
                ((void (*)()) (g_libGTASA + (VER_x32 ? 0x0059CFC0 + 1 : 0x6C13F0)))(); // CBirds::Update()
                ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005C03E4 + 1 : 0x6E4A7C)))(); // CSpecialFX::Update()
            }
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005B28D8 + 1 : 0x6D6E34)))(); // CPostEffects::Update()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x0041EF78 + 1 : 0x502ADC)))(); // CTimeCycle::Update()

            ((void (*)(CCamera*)) (g_libGTASA + (VER_x32 ? 0x003DC7D0 + 1 : 0x4BAB78)))(&TheCamera); // CCamera::Process()

            CHook::CallFunction<void>(g_libGTASA+(VER_x32 ? 0x307D8C+1:0x3CD630));// CGameLogic::Update()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A3E40 + 1 : 0x6C75E4)))(); // CCoronas::DoSunAndMoon()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005A22C8 + 1 : 0x6C5BE0)))(); // CCoronas::Update()
            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x005BD370 + 1 : 0x6E1BC4)))(); // CShadows::UpdatePermanentShadows()

            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x002CA3A4 + 1 : 0x38B6DC)))(); // CCustomBuildingRenderer::Update()

            auto temp = TheCamera.m_pRwCamera;

            auto g_fx = *(uintptr_t *) (g_libGTASA + (VER_x32 ? 0x00820520 : 0xA062A8));
            ((void (*)(uintptr_t*, RwCamera*, float )) (g_libGTASA + (VER_x32 ? 0x00363DE0 + 1 : 0x433F48)))(&g_fx, temp, CTimer::ms_fTimeStep / 50.0f); // Fx_c::Update

            auto g_breakMan = (uintptr_t *) (g_libGTASA + (VER_x32 ? 0x0099DD14 : 0xC31CF0));
            ((void (*)(uintptr_t*, float )) (g_libGTASA + (VER_x32 ? 0x0045267C + 1 : 0x53AFDC)))(g_breakMan, CTimer::ms_fTimeStep); // BreakManager_c::Update

            ((void (*)()) (g_libGTASA + (VER_x32 ? 0x00596540 + 1 : 0x6BB3A8)))(); // CWaterLevel::PreRenderWater()
        }

        static bool once = false;
        if (!once)
        {
            CCrossHair::Init();
            once = true;
        }

    } catch (const std::exception& e) {
        FLog("Exception in CGame::Process: %s", e.what());
    } catch (...) {
        FLog("Unknown exception in CGame::Process");
    }
}

void CGame::InjectHooks()
{
    CHook::Redirect("_ZN5CGame22InitialiseOnceBeforeRWEv", &CGame::InitialiseOnceBeforeRW);
    CHook::Redirect("_ZN5CGame20InitialiseRenderWareEv", &CGame::InitialiseRenderWare);

    CHook::Redirect("_ZN5CGame7ProcessEv", &CGame::Process);
    // CHook::InlineHook("_ZN5CGame7ProcessEv", &CGame__Process_hook, &CGame__Process);

    CHook::Redirect("_ZN5CGame5Init2EPKc", &Init2);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x00678C38 : 0x84F8A0), &CGame::currArea);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x006796E8 : 0x850DF0), &CGame::m_pWorkingMatrix1);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00677B38 : 0x84D6A0), &CGame::m_pWorkingMatrix2);
}


bool CGame::CanSeeOutSideFromCurrArea() {
    return currArea == AREA_CODE_NORMAL_WORLD;
}

void CGame::SetGameInputEnabled(bool b) {
    m_bInputEnable = b;
}

#include "Draw.h"
#include "World.h"
#include "Radar.h"
#include "Animation/AnimManager.h"
bool CGame::Init2(const char *pDatFile)
{
    //std::ranges::for_each(CWorld::Players, [](auto& info) {
    //    info.Clear();
    //});
    CHook::CallFunction<void>("_ZN11CWaterLevel20WaterLevelInitialiseEv"); // CWaterLevel::WaterLevelInitialise();
    CDraw::SetFOV(120.0);
    CDraw::ms_fLODDistance = 500.0f;
    if (!CHook::CallFunction<bool>("_ZN18CCustomCarPlateMgr10InitialiseEv")) {
        FLog("CCustomCarPlateMgr not init");
        return false;
    }
    CHook::CallFunction<void>("_ZN10CStreaming15LoadInitialPedsEv"); // CStreaming::LoadInitialPeds();
    CStreaming::LoadAllRequestedModels(false);
    CAnimManager::LoadAnimFiles();
    CHook::CallFunction<void>("_ZN10CStreaming18LoadInitialWeaponsEv"); // CStreaming::LoadInitialWeapons();
    CStreaming::LoadAllRequestedModels(false);
    CHook::CallFunction<void>("_ZN8CPedType10InitialiseEv"); // CPed::Initialise();
    CRadar::Initialise();
    CRadar::LoadTextures();
    CHook::CallFunction<void>("_ZN7CWeapon17InitialiseWeaponsEv"); // CWeapon::InitialiseWeapons();
    CWorld::PlayerInFocus = 0;
    CHook::CallFunction<void>("_ZN8CCoronas4InitEv"); // CCoronas::Init();
    CHook::CallFunction<void>("_ZN8CShadows4InitEv"); // CShadows::Init();
    CHook::CallFunction<void>("_ZN14CWeaponEffects4InitEv"); // CWeaponEffects::Init();
    CHook::CallFunction<void>("_ZN10CSkidmarks4InitEv"); // CSkidmarks::Init();
    CHook::CallFunction<void>("_ZN6CGlass4InitEv"); // CGlass::Init();
    CHook::CallFunction<void>("_ZN11CTheScripts4InitEv");
    CHook::CallFunction<void>("_ZN6CClock10InitialiseEj", 0x3E8u); // CClock::Initialise(uint32 nMillisecondsPerGameMinute);
    CHook::CallFunction<void>("_ZN13CMovingThings4InitEv"); // CMovingThings::Init();

    CHook::CallFunction<void>("_ZN6CStats4InitEv");

    CHook::CallFunction<void>("_ZN7CClouds4InitEv"); // CClouds::Init();
    CHook::CallFunction<void>("_ZN10CSpecialFX4InitEv"); // CSpecialFX::Init();
    CHook::CallFunction<void>("_ZN13CWaterCannons4InitEv"); // CWaterCannons::Init();

    return true;
}