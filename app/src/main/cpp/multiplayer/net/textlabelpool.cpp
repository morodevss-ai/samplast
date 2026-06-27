#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "game/World.h"
#include <sstream>
#include <cstring>

extern CGame* pGame;
extern CNetGame* pNetGame;

// 0.3.7
C3DTextLabelPool::C3DTextLabelPool()
{
    for (int i = 0; i < MAX_TEXT_LABELS; i++) {
        m_TextLabels[i] = nullptr;

        m_TextLabelLines[i].clear();
        m_TextLabelLineWidths[i].clear();
        m_TextLabelFontSize[i] = 0.0f;
        m_TextLabelLastLosTick[i] = 0;
        m_TextLabelLastLosResult[i] = false;

        m_bSlotUsed[i] = false;
    }
}
// 0.3.7
C3DTextLabelPool::~C3DTextLabelPool()
{
    for (int i = 0; i < MAX_TEXT_LABELS; i++) {
        if (m_bSlotUsed[i]) {
            this->ClearLabel(i);
        }
    }
}
// 0.3.7
void C3DTextLabelPool::NewLabel(uint16_t wLabelId, TEXT_LABEL* pLabel) {

    if (wLabelId < MAX_TEXT_LABELS) {

        if (m_TextLabels[wLabelId])
        {
            delete m_TextLabels[wLabelId];
            m_TextLabels[wLabelId] = nullptr;
            m_bSlotUsed[wLabelId] = false;
        }

        TEXT_LABEL* pTextLabel = new TEXT_LABEL;
        pTextLabel->text = Encoding::cp2utf(pLabel->text);
        m_TextLabelLines[wLabelId].clear();
        m_TextLabelLineWidths[wLabelId].clear();
        m_TextLabelFontSize[wLabelId] = 0.0f;
        m_TextLabelLastLosTick[wLabelId] = 0;
        m_TextLabelLastLosResult[wLabelId] = false;
        {
            std::stringstream ss(pTextLabel->text);
            std::string line;
            while (std::getline(ss, line, '\n'))
            {
                m_TextLabelLines[wLabelId].push_back(line);
            }
        }

        pTextLabel->dwColor = pLabel->dwColor;
        pTextLabel->vecPos.x = pLabel->vecPos.x;
        pTextLabel->vecPos.y = pLabel->vecPos.y;
        pTextLabel->vecPos.z = pLabel->vecPos.z;
        pTextLabel->fDistance = pLabel->fDistance;
        pTextLabel->bTestLOS = pLabel->bTestLOS;
        pTextLabel->playerId = pLabel->playerId;
        pTextLabel->vehicleId = pLabel->vehicleId;

        m_TextLabels[wLabelId] = pTextLabel;
        m_bSlotUsed[wLabelId] = true;
    }
}
// 0.3.7
void C3DTextLabelPool::ClearLabel(uint16_t wLabelId) {
    if (wLabelId < 0 || wLabelId >= MAX_TEXT_LABELS)
    {
        return;
    }
    m_bSlotUsed[wLabelId] = false;
    if (m_TextLabels[wLabelId])
    {
        delete m_TextLabels[wLabelId];
        m_TextLabels[wLabelId] = nullptr;
    }
    m_TextLabelLines[wLabelId].clear();
    m_TextLabelLineWidths[wLabelId].clear();
    m_TextLabelFontSize[wLabelId] = 0.0f;
    m_TextLabelLastLosTick[wLabelId] = 0;
    m_TextLabelLastLosResult[wLabelId] = false;
}

void C3DTextLabelPool::DrawVehiclesInfo(ImGuiRenderer* renderer)
{
    if (!renderer) return;
    if (!pNetGame) return;

    CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
    if (!pVehiclePool) return;

    static char szTextLabel[256];
    float fontSize = UISettings::fontSize() / 2;

    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (pVehiclePool->GetSlotState(i))
        {
            CVehicle *pVehicle = pVehiclePool->GetAt(i);
            if (pVehicle && pVehicle->m_pVehicle && pVehicle->m_pVehicle->IsAdded())
            {
                CVector screenPos;
                CVector vehiclePos = pVehicle->m_pVehicle->GetPosition();

                CVector vPos;
                vPos.x = vehiclePos.x;
                vPos.y = vehiclePos.y;
                vPos.z = vehiclePos.z;

                CVector vecOut;
                // CSprite::CalcScreenCoors
                ((void (*)(CVector *, CVector *, float *, float *, bool, bool)) (g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(
                        &vPos, &vecOut, 0, 0, 0, 0);

                if (vecOut.z < 1.0f) continue;

                RwMatrix matVehicle = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();

                sprintf(szTextLabel,
                        "[id: %d | model: %d | subtype: 0 | Health: %.1f]\n"
                        "Distance: %.2fm\n"
                        "cPos: %.3f, %.3f, %.3f\n"
                        "sPos: %.3f, %.3f, %.3f",
                        i,
                        pVehicle->m_pVehicle->m_nModelIndex,
                        pVehicle->GetHealth(),
                        pVehicle->m_pVehicle->GetDistanceFromCamera(),
                        matVehicle.pos.x,
                        matVehicle.pos.y,
                        matVehicle.pos.z,
                        pVehiclePool->m_vecSpawnPos[i].x,
                        pVehiclePool->m_vecSpawnPos[i].y,
                        pVehiclePool->m_vecSpawnPos[i].z
                );

                std::stringstream ss(szTextLabel);
                std::string line;
                std::vector<std::string> lines;
                while (std::getline(ss, line, '\n'))
                {
                    lines.push_back(line);
                }

                float startY = vecOut.y;
                for (const auto& textLine : lines)
                {
                    float width = renderer->calculateTextSize(textLine, fontSize).x;
                    renderer->drawText(ImVec2(vecOut.x - (width / 2), startY),
                                       __builtin_bswap32(0xf48fb1FF), 
                                       textLine,
                                       true,
                                       fontSize);
                    startY += fontSize;
                }
            }
        }
    }
}

void C3DTextLabelPool::Render(ImGuiRenderer* renderer)
{
    CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
    if(!pPlayerPed) return;
    if(!pNetGame) return;

    if (CGame::m_bDl_enabled && renderer) {
        DrawVehiclesInfo(renderer);
    }

    CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
    if (!pPlayerPool) return;
    CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
    const uint32_t now = GetTickCount();

    for (int i = 0; i < MAX_TEXT_LABELS; i++)
    {
        if (m_bSlotUsed[i]) {
            TEXT_LABEL *pTextLabel = m_TextLabels[i];

            CVector vecTextPos = pTextLabel->vecPos;

            if (pTextLabel->playerId != INVALID_PLAYER_ID) {
                if (pTextLabel->playerId == pPlayerPool->GetLocalPlayerID()) continue;

                if (pPlayerPool->GetSlotState(pTextLabel->playerId)) {
                    CRemotePlayer *pPlayer = pPlayerPool->GetAt(pTextLabel->playerId);
                    if (pPlayer && pPlayer->GetDistanceFromLocalPlayer() < pTextLabel->fDistance) {
                        CPlayerPed *pRemotePed = pPlayer->GetPlayerPed();
                        if (pRemotePed && pRemotePed->m_pPed->IsAdded()) {
                            CVector matBone;
                            pRemotePed->GetBonePosition(8, &matBone);

                            vecTextPos.x = matBone.x + pTextLabel->vecPos.x;
                            vecTextPos.y = matBone.y + pTextLabel->vecPos.y;
                            vecTextPos.z = matBone.z + 0.23 + pTextLabel->vecPos.z;

                            this->Draw(renderer, pTextLabel, static_cast<uint16_t>(i), vecTextPos, pTextLabel->dwColor, pPlayerPed, now);

                        }
                    }
                }
            }
            if (pTextLabel->vehicleId != INVALID_VEHICLE_ID) {
                if (pVehiclePool && pVehiclePool->GetSlotState(pTextLabel->vehicleId)) {
                    CVehicle *pVehicle = pVehiclePool->GetAt(pTextLabel->vehicleId);
                    if (pVehicle && pVehicle->m_pVehicle->IsAdded() &&
                        pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() < pTextLabel->fDistance) {
                        RwMatrix matVehicle = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();

                        vecTextPos.x = matVehicle.pos.x + pTextLabel->vecPos.x;
                        vecTextPos.y = matVehicle.pos.y + pTextLabel->vecPos.y;
                        vecTextPos.z = matVehicle.pos.z + pTextLabel->vecPos.z;

                        this->Draw(renderer, pTextLabel, static_cast<uint16_t>(i), vecTextPos, pTextLabel->dwColor, pPlayerPed, now);
                    }
                }
            }

            if (pPlayerPed->m_pPed->GetDistanceFromPoint(pTextLabel->vecPos.x, pTextLabel->vecPos.y, pTextLabel->vecPos.z) <= pTextLabel->fDistance)
                this->Draw(renderer, pTextLabel, static_cast<uint16_t>(i), vecTextPos, pTextLabel->dwColor, pPlayerPed, now);
        }
    }
}

void C3DTextLabelPool::Draw(ImGuiRenderer* renderer, TEXT_LABEL* label, uint16_t labelId, CVector vecPos,
                            uint32_t dwColor, CPlayerPed* localPlayerPed, uint32_t now)
{
    CVector vPos;
    vPos.x = vecPos.x;
    vPos.y = vecPos.y;
    vPos.z = vecPos.z;

    static CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    static constexpr uint32_t kLosCacheMs = 100;

    bool hasLos = true;
    if (label->bTestLOS) {
        CAMERA_AIM *pCam = GameGetInternalAim();
        if (!pCam)
        {
            return;
        }

        if (now - m_TextLabelLastLosTick[labelId] >= kLosCacheMs)
        {
            m_TextLabelLastLosResult[labelId] = CWorld::GetIsLineOfSightClear(
                    vecPos, TheCamera.GetPosition(), true, false, false, true, false, false, false);
            m_TextLabelLastLosTick[labelId] = now;
        }
        hasLos = m_TextLabelLastLosResult[labelId];
    }

    if (!label->bTestLOS || hasLos) {
        if (localPlayerPed->m_pPed->GetDistanceFromPoint(vecPos.x, vecPos.y, vecPos.z) <= label->fDistance) {
            CVector vecOut;
            // CSprite::CalcScreenCoors
            ((void (*)(CVector *, CVector *, float *, float *, bool, bool)) (g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(
                    &vPos, &vecOut, 0, 0, 0, 0);
            if (vecOut.z < 1.0f) return;

            const auto& lines = m_TextLabelLines[labelId];
            if (lines.empty()) return;

            const float fontSize = UISettings::fontSize() / 2;
            auto& widths = m_TextLabelLineWidths[labelId];
            if (m_TextLabelFontSize[labelId] != fontSize || widths.size() != lines.size()) {
                widths.clear();
                widths.reserve(lines.size());
                for (const auto& line : lines) {
                    widths.push_back(renderer->calculateTextSize(line, fontSize).x);
                }
                m_TextLabelFontSize[labelId] = fontSize;
            }

            for (size_t idx = 0; idx < lines.size(); ++idx) {
                const auto& line = lines[idx];
                const float width = widths[idx];
                renderer->drawText(ImVec2(vecOut.x - (width / 2), vecOut.y),
                                   __builtin_bswap32(dwColor | (0x000000FF)), line, true,
                                   fontSize);
                vecOut.y += fontSize;
            }
        }
    }
}