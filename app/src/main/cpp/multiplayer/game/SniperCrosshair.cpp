#include "SniperCrosshair.h"
#include "game.h"
#include "net/netgame.h"
#include "playerped.h"
#include "sprite2d.h"
#include "common.h"

extern CNetGame *pNetGame;

void CSniperCrosshair::Render()
{
    if(!pNetGame || !pNetGame->GetPlayerPool() || !pNetGame->GetPlayerPool()->GetLocalPlayer())
        return;

    auto pPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
    if(!pPed || !pPed->m_pPed) return;

    // Draw a crosshair if the player is aiming, but not in sniper mode
    if (IS_TARGETING(pPed->m_pPed) && pPed->GetCameraMode() != 4 && pPed->GetCameraMode() != 53)
    {
        float x = RsGlobal->maximumWidth / 2.0f;
        float y = RsGlobal->maximumHeight / 2.0f;
        float size = 4.0f;

        CSprite2d::DrawRect(CRect(x - size, y - size, x + size, y + size), CRGBA(255, 255, 255, 200));
    }
}