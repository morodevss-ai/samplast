//
// Created by wn on 04.08.2025.
//

#include "Draw.h"
#include "../vendor/armhook/patch.h"

void CDraw::InjectHooks() {
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006779A0 : 0x84D378), &ms_fFarClipZ);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00676178 : 0x84A370), &ms_fNearClipZ);

    // 04.08.2025 - new limit: lod distance (mobile def: 500.0 \ pc def: 300.0 \ new in MAX_LOD_DISTANCE)
    CHook::Write(g_libGTASA + (VER_x32? 0xA26A8C : 0xCC7EFC), &ms_fLODDistance);
}

void CDraw::SetFOV(float fov) {
    CHook::CallFunction<void>("_ZN5CDraw6SetFOVEfb", fov, false);
}