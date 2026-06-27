//
// Created by bkuzn on 10.01.2026.
//
#include "graphics/CSkyBox.h"
#include "main.h"
#include "game/game.h"
#include "patch.h"
#include "game/Coronas.h"
#include "net/netgame.h"
#include "game/Mobile/MobileSettings/MobileSettings.h"

extern CGame* pGame;
extern CNetGame *pNetGame;


CObject* CSkyBox::m_pSkyBox = nullptr;
RwTexture* pSkyTexture = nullptr;
float rx = 0.0f;

void CSkyBox::Process() {
    if(!CMobileSettings::isSky) {
        if(m_pSkyBox) {
            m_pSkyBox->m_pEntity->Remove();
        }
        return;
    }

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    if (!m_pSkyBox) {
        CVector vec = {TheCamera.m_matrix->m_pos.x, TheCamera.m_matrix->m_pos.y, TheCamera.m_matrix->m_pos.z};
        CVector vec2 = {0.0f, 0.0f, 0.0f};
        m_pSkyBox = pGame->NewObject(18500, vec, vec2, 1.0f);
    } else {
        m_pSkyBox->MoveTo(TheCamera.m_matrix->m_pos.x, TheCamera.m_matrix->m_pos.y, TheCamera.m_matrix->m_pos.z, 1.0f, 0.0f, 0.0f, rx);
        rx = rx + 0.01f;
m_pSkyBox->SetPos(pGame->FindPlayerPed()->m_pPed->m_matrix->m_pos.x + 10.0f, pGame->FindPlayerPed()->m_pPed->m_matrix->m_pos.y + 10.0f, pGame->FindPlayerPed()->m_pPed->m_matrix->m_pos.z  + 10.0f);
}
}