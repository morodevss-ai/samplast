#include "Radar.h"
#include "../vendor/armhook/patch.h"

extern uintptr (*CRadar__SetCoordBlip)(eBlipType blipType, CVector pos, uint32 nColour, eBlipDisplay r5, char *name);

uintptr CRadar::SetCoordBlip(eBlipType blipType, CVector pos, eBlipDisplay DispFlag) {
    constexpr auto nColour = 0; // not used

    return CRadar__SetCoordBlip(blipType, pos, nColour, DispFlag, "name");
}

/*!
 * @brief Clear a blip
 */
void CRadar::ClearBlip(tBlipHandle blip) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x004429E8 + 1 : 0x527C60), blip);
}

/*!
 * @brief Transforms a real coordinate to radar coordinate.
 */
void CRadar::TransformRealWorldPointToRadarSpace(CVector2D* out, const CVector2D* in) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0043F6E4 + 1 : 0x524AD0), out, in);
}

/*!
 * @brief Transforms a radar point to screen.
 * @brief Unhooked by default for now. Causes `DrawRadarSection` to crash.
 */
void CRadar::TransformRadarPointToScreenSpace(CVector2D* out, const CVector2D* in) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0043F62C + 1 : 0x524A30), out, in);
}

/*!
 * @brief Limits a 2D vector to the radar. (which is a unit circle)
 * @brief This function does not effect the vector if the map is being drawn.
 * @param point The vector to be limited.
 * @returns Magnitude of the vector before limiting.
 */
float CRadar::LimitRadarPoint(CVector2D *point) {
    return CHook::CallFunction<float>(g_libGTASA + (VER_x32 ? 0x0043F760 + 1 : 0x524B2C), point);
}

void CRadar::LimitToMap(float* x, float* y) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x004424E4 + 1 : 0x5277EC), x, y);
}

void CRadar::ClearBlipForEntity(eBlipType BlpType, int32_t IndexInPool) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00442990 + 1 : 0x527C00), BlpType, IndexInPool);
}

int CRadar::SetEntityBlip(eBlipType BlpType, int32_t IndexInPool, uint32_t nColour, eMarkerDisplay DispFlag, char* pScriptName) {
    return CHook::CallFunction<int>(g_libGTASA + (VER_x32 ? 0x00442900 + 1 : 0x527B68), BlpType, IndexInPool, nColour, DispFlag);
}

void CRadar::ChangeBlipScale(int32_t nIndex, int32_t NewScale) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00442B10 + 1 : 0x527D74), nIndex, NewScale);
}

void CRadar::ChangeBlipDisplay(int32_t nIndex, eMarkerDisplay DispFlag) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00442B80 + 1 : 0x527DD4), nIndex, DispFlag);
}

void CRadar::ChangeBlipColour(int32_t nIndex, uint32_t nNewColour) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00442A58 + 1 : 0x527CD4), nIndex, nNewColour);
}

void CRadar::SetBlipSprite(int32_t nIndex, int32_t BlipSprite) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00442BE0 + 1 : 0x527E1C), nIndex, BlipSprite);
}

void CRadar::Initialise() {
    CHook::CallFunction<void>("_ZN6CRadar10InitialiseEv");
}

void CRadar::LoadTextures() {
    CHook::CallFunction<void>("_ZN6CRadar12LoadTexturesEv");
}
