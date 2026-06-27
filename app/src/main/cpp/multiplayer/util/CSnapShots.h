//
// Created by Error on 18.10.2024.
//

#ifndef BRSELL_CSNAPSHOTS_H
#define BRSELL_CSNAPSHOTS_H
#include "../main.h"
#include "../game/game.h"
#include "../game/RW/RenderWare.h"
#include "CRenderTarget.h"
#include "CSnapShotWrapper.h"

class CSnapShots {
    static CRenderTarget* m_pRenderTarget;
public:
    static void Initialise();
    static RwTexture* CreatePedSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom);
    static RwTexture* CreateVehicleSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom, int dwColor1, int dwColor2);
    static RwTexture* CreateObjectSnapShot(int iModel, uint32_t dwColor, VECTOR* vecRot, float fZoom);
    static RwTexture* CreateTextureSnapShot(const char* tex);
    static RwTexture* CreatePlayerSnapShot(VECTOR* vecRot, float fZoom);
};


#endif //BRSELL_CSNAPSHOTS_H
