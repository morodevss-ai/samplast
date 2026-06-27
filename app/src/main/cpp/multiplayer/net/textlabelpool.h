#pragma once

#include "../gui/gui.h"
#include <vector>
#include <map>

class CPlayerPed;

#pragma pack(push, 1)
typedef struct _TEXT_LABEL
{
    std::string text;
    uint32_t dwColor;
    CVector vecPos;
    float fDistance;
    uint8_t bTestLOS;
    PLAYERID playerId;
    VEHICLEID vehicleId;
} TEXT_LABEL;
#pragma pack(pop)

class C3DTextLabelPool
{
public:
    C3DTextLabelPool();
    ~C3DTextLabelPool();

    bool GetSlotState(uint16_t wLabelId) {
        if (wLabelId < MAX_TEXT_LABELS && m_bSlotUsed[wLabelId]) {
            return true;
        }

        return false;
    }

    void NewLabel(uint16_t wLabelId, TEXT_LABEL* label);
    void ClearLabel(uint16_t wLabelId);

    void Render(ImGuiRenderer* renderer);
    void DrawVehiclesInfo();

private:
    TEXT_LABEL	*m_TextLabels[MAX_TEXT_LABELS];
    std::vector<std::string> m_TextLabelLines[MAX_TEXT_LABELS];
    std::vector<float> m_TextLabelLineWidths[MAX_TEXT_LABELS];
    float m_TextLabelFontSize[MAX_TEXT_LABELS];
    uint32_t m_TextLabelLastLosTick[MAX_TEXT_LABELS];
    bool m_TextLabelLastLosResult[MAX_TEXT_LABELS];
    bool		m_bSlotUsed[MAX_TEXT_LABELS];

    void Draw(ImGuiRenderer *renderer, TEXT_LABEL *label, uint16_t labelId, CVector vecPos,
              uint32_t dwColor, CPlayerPed* localPlayerPed, uint32_t now);

    void DrawVehiclesInfo(ImGuiRenderer *renderer);
};