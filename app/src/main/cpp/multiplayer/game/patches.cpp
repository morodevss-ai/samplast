#include "../main.h"
#include "../game/game.h"
#include "../vendor/armhook/patch.h"
#include "Mobile/MobileMenu/MobileMenu.h"
#include "vehicleColoursTable.h"
#include "../settings.h"
extern CSettings* pSettings;

VehicleAudioPropertiesStruct VehicleAudioProperties[20000];
#include "game.h"
#include "Streaming.h"
#include "TxdStore.h"
#include "World.h"
#include "net/netgame.h"

extern CGame* pGame;

static std::unordered_map<std::string, std::vector<char>> g_fileCache;

void CacheFile(const char* filename)
{
    if(g_fileCache.find(filename) != g_fileCache.end()) return;

    FILE* f = fopen(filename, "rb");
    if(f)
    {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::vector<char> buffer(size);
        fread(buffer.data(), 1, size, f);
        fclose(f);

        g_fileCache[filename] = std::move(buffer);
    }
}

void fixUVAnim(bool enable) {
#if VER32
    CHook::WriteMemory(g_libGTASA + 0x6BD1E4, (uintptr_t)"\x01", 1);
#else
    CHook::Write(g_libGTASA + 0x89AA88, &enable);

    CHook::NOP(g_libGTASA + 0x25EE0C, 1);
    CHook::NOP(g_libGTASA + 0x25F5EC, 1);
    CHook::NOP(g_libGTASA + 0x25EDC8, 1);
#endif
}

void ApplyFPSPatch(uint8_t fps)
{
    uint8_t targetFPS = fps;
    if (targetFPS < 30) {
        targetFPS = 30;
    } else if (targetFPS > 120) {
        targetFPS = 120;
    }

#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x005E49E0, (uintptr_t)&targetFPS, 1);
    CHook::WriteMemory(g_libGTASA + 0x005E492E, (uintptr_t)&targetFPS, 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x70A38C + 1, &targetFPS, 1);
    CHook::WriteMemory(g_libGTASA + 0x70A43C + 1, &targetFPS, 1);
    CHook::WriteMemory(g_libGTASA + 0x70A458 + 1, &targetFPS, 1);
#endif

    FLog("New fps limit = %d", targetFPS);
}

bool IsLockOnMode()
{
    return 1;
}

bool IsFreeAimMode()
{
    return 0;
}

void ApplyPausePathes() {
    CHook::RET("_ZN6CRadar10DrawLegendEiii");

#if VER_x32
    CHook::NOP(g_libGTASA + 0x2AB4A6, 2);
#else
    CHook::NOP(g_libGTASA + 0x36A190, 1);
#endif
}

void ApplySAMPPatchesInGame()
{
    FLog("Installing patches (ingame)..");

    // CTheZones::ZonesVisited[100]
    memset((void*)(g_libGTASA + (VER_x32 ? 0x0098D252 : 0xC1BF92)), 1, 100);
    // CTheZones::ZonesRevealed
    *(uint32_t*)(g_libGTASA + (VER_x32 ? 0x0098D2B8 : 0xC1BFF8)) = 100;

    // CPlayerPed::CPlayerPed task fix
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x004C36E2, (uintptr_t)"\xE0", 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x5C0BC4, (uintptr_t)"\x34\x00\x80\x52", 4);
#endif

    // radar draw blips
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0043FE5A : 0x52522C), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x004409AE : 0x525E14), 2);

    // no vehicle audio processing
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x00553E96 : 0x674610), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x00561AC2 : 0x682C1C), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0056BED4 : 0x68DD0C), 2);

    // disable in-game radio
    CHook::RET("_ZN20CAERadioTrackManager7ServiceEi");

    // disable place name hud
    CHook::RET("_ZN10CPlaceName7ProcessEv");
    // disable veh name hud
    CHook::RET("_ZN4CHud15DrawVehicleNameEv");

    // карта в меню
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2ABA08 : 0x36A6E8), 2); // текст легенды карты
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2ABA14 : 0x36A6F8), 2); // значки легенды
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2AB4A6 : 0x36A190), 2); // название местности
}

int32_t CWorld__FindPlayerSlotWithPedPointer(CPedGTA* pPlayersPed)
{
    static CPedGTA* lastPed = nullptr;
    static int32_t lastIndex = -1;

    if(lastPed == pPlayersPed && lastIndex != -1)
        return lastIndex;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(CWorld::Players[i].m_pPed == pPlayersPed)
        {
            lastPed = pPlayersPed;
            lastIndex = i;
            return i;
        }
    }

    lastPed = nullptr;
    lastIndex = -1;
    return -1;
}

void ApplyPatches_level0()
{
    FLog("ApplyPatches_level0");

    CHook::Write(g_libGTASA + (VER_x32 ? 0x006783C0 : 0x84E7A8), &CWorld::Players);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679B5C : 0x8516D8), &CWorld::PlayerInFocus);

    CHook::Redirect("_ZN6CWorld28FindPlayerSlotWithPedPointerEPv", &CWorld__FindPlayerSlotWithPedPointer);

    ApplyPausePathes();

    // fix aplha raster
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x001AE8DE, (uintptr_t)"\x01\x22", 2);
    CHook::WriteMemory(g_libGTASA + 0x5A2C04, (uintptr_t)"\x00\x20\x70\x47", 4);
    CHook::NOP(g_libGTASA + 0x3F62D8, 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x23FDE0, (uintptr_t)"\x22\x00\x80\x52", 4);
#endif

    // fixes a weird ass glitches with the timer value clamping
#if VER_x32
    CHook::Write(g_libGTASA + 0x420E40, 0.001f);
#else
    CHook::Write32(g_libGTASA + 0x5045F4, 0xD00010AB);
    CHook::Write32(g_libGTASA + 0x504600, 0xBD41E161);
#endif

#if !VER_x32
    // fix skin vertices
    CHook::WriteMemory(g_libGTASA + 0x266FC8, (uintptr_t)"\x15\x80\xA0\x52", 4);
#else
    CHook::WriteMemory(g_libGTASA + 0x1D16E4, (uintptr_t)"\x4F\xF0\x80\x70", 4);
    CHook::WriteMemory(g_libGTASA + 0x1D16EE, (uintptr_t)"\x4F\xF0\x80\x76", 4);
#endif

    CHook::RET("_ZN6CTrain10InitTrainsEv");
    CHook::RET("_ZN6CTrain18CreateMissionTrainE7CVectorbjPPS_S2_iib");

    CHook::RET("_ZN8CClothes4InitEv");
    CHook::RET("_ZN8CClothes13RebuildPlayerEP10CPlayerPedb");

    CHook::RET("_ZNK35CPedGroupDefaultTaskAllocatorRandom20AllocateDefaultTasksEP9CPedGroupP4CPed");
    CHook::RET("_ZN6CGlass4InitEv");
    CHook::RET("_ZN8CGarages17Init_AfterRestartEv");
    CHook::RET("_ZN6CGangs10InitialiseEv");
    CHook::RET("_ZN5CHeli9InitHelisEv");
    CHook::RET("_ZN11CFileLoader10LoadPickupEPKc");
    CHook::RET("_ZN14CLoadingScreen15DisplayPCScreenEv");

    CHook::RET("_ZN10CSkidmarks6UpdateEv");
    CHook::RET("_ZN10CSkidmarks6RenderEv");

    CHook::RET("_ZN14SurfaceInfos_c16CreatesWheelDustEj");
    CHook::RET("_ZN14SurfaceInfos_c17CreatesWheelSprayEj");

    CHook::RET("_ZN4Fx_c12AddWheelDustEP8CVehicle7CVectorhf");
    CHook::RET("_ZN4Fx_c13AddWheelSprayEP8CVehicle7CVectorhhf");
}

void InstallCustomPatches()
{
    // OSWrapper
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x60A484 : 0x7371E0));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x60A484 : 0x7371E0)), ("x1y2zWrapper?"));

    // NVEvent
    CHook::UnFuck(g_libGTASA + (VER_x32 ? 0x60A5B5 : 0x73740C));
    strcpy((char*)(g_libGTASA + (VER_x32 ? 0x60A5B5 : 0x73740C)), ("x1y2zNVEvent?"));
}

void ApplyGlobalPatches()
{
    FLog("Installing patches..");

    CHook::RET("_ZN17CVehicleModelInfo17SetCarCustomPlateEv");
    CHook::RET("_Z16SaveGameForPause10eSaveTypesPc");

    // цвета иконок
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x004404C0, (uintptr_t)"\x3A\xE0", 2);
    CHook::WriteMemory(g_libGTASA + 0x00440538, (uintptr_t)"\x30\x46", 2);
    CHook::WriteMemory(g_libGTASA + 0x0043FB5E, (uintptr_t)"\x12\xE0", 2);
    CHook::WriteMemory(g_libGTASA + 0x0043FB86, (uintptr_t)"\x48\x46", 2);
#else
    CHook::WriteMemory(g_libGTASA + 0x5258D8, (uintptr_t)"\x22\x00\x00\x14", 4);
    CHook::WriteMemory(g_libGTASA + 0x525960, (uintptr_t)"\xE1\x03\x16\x2A", 4);
    CHook::WriteMemory(g_libGTASA + 0x524F58, (uintptr_t)"\xCC\xFF\xFF\x17", 4);
    CHook::WriteMemory(g_libGTASA + 0x524E88, (uintptr_t)"\xE1\x03\x16\x2A", 4);

    // crash legend
    CHook::NOP(g_libGTASA + 0x36A690, 1);
#endif

    CHook::RET("_ZN12CAudioEngine16StartLoadingTuneEv");

    // DefaultPCSaveFileName
    char* DefaultPCSaveFileName = (char*)(g_libGTASA + (VER_x32 ? 0x006B012C : 0x88CB08));
    memcpy(DefaultPCSaveFileName, "GTASAMP", 8);

#if VER_x32
    CHook::NOP(g_libGTASA + 0x003F61B6, 2);
    CHook::NOP(g_libGTASA + 0x00584884, 2);
    CHook::NOP(g_libGTASA + 0x00584850, 2);
#else
    CHook::NOP(g_libGTASA + 0x004D8700, 1);
    CHook::NOP(g_libGTASA + 0x006A852C, 1);
    CHook::NOP(g_libGTASA + 0x006A84E0, 1);
#endif

    CHook::RET("_ZN17CVehicleRecording4LoadEP8RwStreamii");

    CHook::RET("_ZN18CMotionBlurStreaks6UpdateEv");
    CHook::RET("_ZN7CCamera16RenderMotionBlurEv");

    // disable auto aim
    CHook::RET("_ZN10CPlayerPed22FindWeaponLockOnTargetEv");
    CHook::RET("_ZN10CPlayerPed26FindNextWeaponLockOnTargetEP7CEntityb");
    CHook::RET("_ZN4CPed21SetWeaponLockOnTargetEP7CEntity");

    CHook::RET("_ZN11CPlayerInfo17FindObjectToStealEP4CPed");
    CHook::RET("_ZN26CAEGlobalWeaponAudioEntity21ServiceAmbientGunFireEv");
    CHook::RET("_ZN30CWidgetRegionSteeringSelection4DrawEv");
    CHook::RET("_ZN23CTaskSimplePlayerOnFoot18PlayIdleAnimationsEP10CPlayerPed");
    CHook::RET("_ZN13CCarEnterExit17SetPedInCarDirectEP4CPedP8CVehicleib");
    CHook::RET("_ZN6CRadar10DrawLegendEiii");
    CHook::RET("_ZN6CRadar19AddBlipToLegendListEhi");

    CHook::RET("_ZN11CAutomobile35CustomCarPlate_BeforeRenderingStartEP17CVehicleModelInfo");
    CHook::RET("_ZN11CAutomobile33CustomCarPlate_AfterRenderingStopEP17CVehicleModelInfo");
    CHook::RET("_ZN7CCamera8CamShakeEffff");
    CHook::RET("_ZN7CEntity23PreRenderForGlassWindowEv");
    CHook::RET("_ZN8CMirrors16RenderReflBufferEb");
    CHook::RET("_ZN4CHud23DrawBustedWastedMessageEv");
    CHook::RET("_ZN4CHud14SetHelpMessageEPKcPtbbbj");
    CHook::RET("_ZN4CHud24SetHelpMessageStatUpdateEhtff");
    CHook::RET("_ZN6CCheat16ProcessCheatMenuEv");
    CHook::RET("_ZN6CCheat13ProcessCheatsEv");
    CHook::RET("_ZN6CCheat16AddToCheatStringEc");
    CHook::RET("_ZN6CCheat12WeaponCheat1Ev");
    CHook::RET("_ZN6CCheat12WeaponCheat2Ev");
    CHook::RET("_ZN6CCheat12WeaponCheat3Ev");
    CHook::RET("_ZN6CCheat12WeaponCheat4Ev");
    CHook::RET("_ZN8CGarages14TriggerMessageEPcsts");

    CHook::RET("_ZN5CBoat14ProcessControlEv");
    CHook::RET("_ZN10CGameLogic43SetPlayerWantedLevelForForbiddenTerritoriesEb");
    CHook::RET("_ZN7CWanted14ReportCrimeNowE10eCrimeTypeRK7CVectorb");

    CHook::RET("_ZN11CPopulation6AddPedE8ePedTypejRK7CVectorb");
    CHook::RET("_ZN6CPlane27DoPlaneGenerationAndRemovalEv");

    CHook::RET("_ZN10CEntryExit19GenerateAmbientPedsERK7CVector");
    CHook::RET("_ZN8CCarCtrl31GenerateOneEmergencyServicesCarEj7CVector");
    CHook::RET("_ZN11CPopulation17AddPedAtAttractorEiP9C2dEffect7CVectorP7CEntityi");

    CHook::RET("_ZN7CDarkel26RegisterCarBlownUpByPlayerEP8CVehiclei");
    CHook::RET("_ZN7CDarkel25ResetModelsKilledByPlayerEi");
    CHook::RET("_ZN7CDarkel25QueryModelsKilledByPlayerEii");
    CHook::RET("_ZN7CDarkel27FindTotalPedsKilledByPlayerEi");
    CHook::RET("_ZN7CDarkel20RegisterKillByPlayerEPK4CPed11eWeaponTypebi");

    // crmp: on, samp: off
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0046BE88 : 0x55774C), 1);

    fixUVAnim(true);

#if VER_x32
    CHook::NOP(g_libGTASA + 0x46BDE8, 1);
    CHook::NOP(g_libGTASA + 0x46BDF8, 1);
#else
    CHook::NOP(g_libGTASA + 0x55768C, 1);
    CHook::NOP(g_libGTASA + 0x557690, 1);
#endif

#if VER_x32
    CHook::NOP(g_libGTASA + (VER_2_1 ? 0x0040BF26 : 0x3AC8B2), 2);
    CHook::NOP(g_libGTASA + 0x004C5902, 2);
    CHook::NOP(g_libGTASA + (VER_2_1 ? 0x003F395E : 0x39840A), 2);
    CHook::WriteMemory(g_libGTASA + 0x003F4138, "\x03", 1);
#else
    CHook::NOP(g_libGTASA + 0x5C3258, 1);
    CHook::WriteMemory(g_libGTASA + 0x266FC8, "\xF5\x03\x08\x32", 4);
    CHook::WriteMemory(g_libGTASA + 0x4D644C, "\x1F\x0D\x00\x71", 4);
#endif

    CHook::RET("_ZN10CPlayerPed14AnnoyPlayerPedEb");
    CHook::RET("_ZN11CPopulation15AddToPopulationEffff");

    CHook::RET("_ZN23CAEPedSpeechAudioEntity11AddSayEventEisjfhhh");

    CHook::RET("_ZN8CMirrors16BeforeMainRenderEv");

    CHook::RET("_ZN10CPedGroups7ProcessEv");
    CHook::RET("_ZN21CPedGroupIntelligence7ProcessEv");
    CHook::RET("_ZN19CPedGroupMembership9SetLeaderEP4CPed");
    CHook::RET("_ZN21CPedGroupIntelligence5FlushEv");

    CHook::RET("_ZN8CCarCtrl18GenerateRandomCarsEv");
}

void readVehiclesAudioSettings()
{
    char vehicleModel[50];
    int16_t pIndex = 0;

    char buffer[0xFF];
    sprintf(buffer, "%sSAMP/vehicleAudioSettings.cfg", g_pszStorage);

    CacheFile(buffer);

    if(g_fileCache.find(buffer) == g_fileCache.end())
        return;

    memset(VehicleAudioProperties, 0x00, sizeof(VehicleAudioProperties));

    VehicleAudioPropertiesStruct CurrentVehicleAudioProperties;
    memset(&CurrentVehicleAudioProperties, 0x0, sizeof(VehicleAudioPropertiesStruct));

    const auto& fileData = g_fileCache[buffer];
    std::stringstream ss(std::string(fileData.begin(), fileData.end()));
    std::string line;

    while(std::getline(ss, line))
    {
        if (strncmp(line.c_str(), ";the end", 8) == 0)
            break;

        if (line[0] == ';')
            continue;

        sscanf(line.c_str(), "%s %d %d %d %d %f %f %d %f %d %d %d %d %f",
               vehicleModel,
               &CurrentVehicleAudioProperties.VehicleType,
               &CurrentVehicleAudioProperties.EngineOnSound,
               &CurrentVehicleAudioProperties.EngineOffSound,
               &CurrentVehicleAudioProperties.field_4,
               &CurrentVehicleAudioProperties.field_5,
               &CurrentVehicleAudioProperties.field_6,
               &CurrentVehicleAudioProperties.HornTon,
               &CurrentVehicleAudioProperties.HornHigh,
               &CurrentVehicleAudioProperties.DoorSound,
               &CurrentVehicleAudioProperties.RadioNum,
               &CurrentVehicleAudioProperties.RadioType,
               &CurrentVehicleAudioProperties.field_14,
               &CurrentVehicleAudioProperties.field_16);

        ((void (*)(const char* thiz, int16_t* a2))(g_libGTASA + 0x385E38 + 1))(vehicleModel, &pIndex);
        memcpy(&VehicleAudioProperties[pIndex-400], &CurrentVehicleAudioProperties, sizeof(VehicleAudioPropertiesStruct));
    }
}

void PreloadTextures()
{
    FLog("Preloading textures for optimization...");

    const char* commonTextures[] = {
            "radar", "hud", "fonts", "vehicle", "ped", "weapon"
    };

    for(const auto& tex : commonTextures)
    {
        // Preloading textures
    }
}

void ClearFileCache()
{
    g_fileCache.clear();
    FLog("File cache cleared");
}

void InstallVehicleEngineLightPatches()
{
    CHook::WriteMemory(g_libGTASA + 0x591272, (uintptr_t)"\x02", 1);
    CHook::WriteMemory(g_libGTASA + 0x59128E, (uintptr_t)"\x02", 1);
}