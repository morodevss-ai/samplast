//
// Created by Error on 18.10.2024.
//

#include "CSnapShots.h"
#include "net/netgame.h"
#include "CRenderTarget.h"
#include "java/jniutil.h"
#include "game/Models/ModelInfo.h"
#include "game/sprite2d.h"
#include "game/Plugins/RpAnimBlendPlugin/RpAnimBlend.h"
#include "game/Core/Vector2D.h"
#include "game/Core/Vector.h"
#include "game/game.h"
#include <vector>

CRenderTarget* CSnapShots::m_pRenderTarget = nullptr;
extern CJavaWrapper* g_pJavaWrapper;
extern CGame *pGame;
extern CNetGame *pNetGame;

RwTexture* CSnapShots::CreatePedSnapShot(int iModel, uint32_t dwColor, VECTOR *vecRot, float fZoom) {
    FLog("CreatePedSnapShot: %d, %f, %f, %f", iModel, vecRot->X, vecRot->Y, vecRot->Z);

    CPlayerPed *pPed = new CPlayerPed(208, 0, 0.0f, 0.0f, 0.0f, 0.0f);

    float posZ = iModel == 162 ? 50.15f : 50.05f;
    float posY = fZoom * -2.25f;
    pPed->m_pPed->SetPosn(0.0f, posY, posZ);
    pPed->SetModelIndex(iModel);
    //pPed->m_pPed->SetGravityProcessing(false);
    pPed->m_pPed->SetCollisionChecking(false);

    RwMatrix mat = pPed->m_pPed->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->X != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->X);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->Y != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->Y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->Z != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->Z);
    }

    pPed->m_pPed->SetMatrix((CMatrix&)mat);

    pPed->m_pPed->UpdateRW();

    pPed->m_pPed->Add();
    FLog("биляж");
    m_pRenderTarget->Begin(512, 512, 0x00000000, false);
    FLog("биляж 2");
    RpAnimBlendClumpUpdateAnimations(pPed->m_pPed->m_pRwClump, 100.0f, 1);
    RenderEntity(pPed->m_pPed);
    FLog("биляж");

    pPed->m_pPed->Remove();

    delete pPed;

    return m_pRenderTarget->End();
}

RwTexture *
CSnapShots::CreateVehicleSnapShot(int iModel, uint32_t dwColor, VECTOR *vecRot, float fZoom,
                                  int dwColor1, int dwColor2) {
    FLog("CreateVehicleSnapShot: %d, %f, %f, %f", iModel, vecRot->X, vecRot->Y, vecRot->Z);



    CVehicle *pVehicle = new CVehicle(iModel, 0.0f, 0.0f, 50.0f, 0.0f, false, false);

    FLog("NORM load");

    //pVehicle->m_pVehicle->SetGravityProcessing(false);
    pVehicle->m_pVehicle->SetCollisionChecking(false);

    float fRadius = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundRadius();
    FLog("это?");
    float posY = (-1.0 - (fRadius + fRadius)) * fZoom;

    if(pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_BOAT)
    {
        posY = -5.5 - fRadius * 2.0;
    }

    pVehicle->m_pVehicle->SetPosn(0.0f, posY, 50.0f);

    if(dwColor1 != 0xFFFF && dwColor2 != 0xFFFF)
        pVehicle->SetColor(dwColor1, dwColor2);

    RwMatrix mat = pVehicle->m_pVehicle->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->X != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->X);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->Y != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->Y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->Z != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->X);
    }
    FLog("SWnapShot veh pos %f %f %f", mat.pos.x, mat.pos.y, mat.pos.z);

    pVehicle->m_pVehicle->SetMatrix((CMatrix&)mat);
    pVehicle->m_pVehicle->UpdateRW();


    FLog("biiih");
    pVehicle->m_pVehicle->Add();
    FLog("biiih");
    m_pRenderTarget->Begin(512, 512, 0x00000000, false);///C69C7B

    RenderEntity(pVehicle->m_pVehicle);
    pVehicle->m_pVehicle->Remove();
    delete pVehicle;

    return m_pRenderTarget->End();
}



void CSnapShots::Initialise() {
	
    m_pRenderTarget = new CRenderTarget();//512, 512, false, 0xC69C7B
    FLog("cerash?");
    m_pRenderTarget->Initialise();
	
}

RwTexture *
CSnapShots::CreateObjectSnapShot(int iModel, uint32_t dwColor, VECTOR *vecRot, float fZoom) {
    if (iModel == 1373 || iModel == 3118 || iModel == 3552 || iModel == 3553)
        iModel = 18631;
    FLog("CreateObjectSnapShot: %d, %f, %f, %f", iModel, vecRot->X, vecRot->Y, vecRot->Z);
    bool bNeedRemoveModel = false;
    if (!pGame->IsModelLoaded(iModel))
    {
        pGame->RequestModel(iModel);
        pGame->LoadRequestedModels();
        while (!pGame->IsModelLoaded(iModel)) sleep(1);
        bNeedRemoveModel = true;
    }

    auto pRwObject = ModelInfoCreateInstance(iModel);
    if (pRwObject == nullptr) {
        FLog("pRwObject no rw object");
        return nullptr;
    }

    CVector vec;
    vec.x = 0.0f;
    vec.y = 0.0f;
    vec.z = 0.0f;

    float fRadius = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundRadius();
    CVector vecCenter = CModelInfo::GetModelInfo(iModel)->m_pColModel->GetBoundCenter();
    RwFrame* parent = static_cast<RwFrame *>(pRwObject->parent);
    if(!parent) return nullptr;
    fZoom = (-0.1f - fRadius * 2.25f) * fZoom;
    if (parent)
    {
        RwV3d v = {
                -vecCenter.x + vecRot->X,
                fZoom + vecRot->Y,
                50.0f - vecCenter.z - vecRot->Z
        };
        RwFrameTranslate(parent, &v, rwCOMBINEPRECONCAT);
        if (iModel == 18631) {
            RwFrameRotate(parent, &vec, 180.0f,rwCOMBINEPRECONCAT);
        }
        else
        {
            if (vecRot->X != 0.0f) {
                RwFrameRotate(parent, &vec, vecRot->X,rwCOMBINEPRECONCAT);
            }
            if (vecRot->Y != 0.0f) {
                RwFrameRotate(parent, &vec, vecRot->Y,rwCOMBINEPRECONCAT);
            }
            if (vecRot->Z != 0.0f) {
                RwFrameRotate(parent, &vec, vecRot->Z,rwCOMBINEPRECONCAT);
            }
        }
    }

    m_pRenderTarget->Begin(512, 512, 0x00000000, false);

    RenderClumpOrAtomic((uintptr_t)pRwObject);

    DestroyAtomicOrClump(reinterpret_cast<uintptr_t>(pRwObject));

    if (bNeedRemoveModel) {
        pGame->RemoveModel(iModel, false);
    }

    return m_pRenderTarget->End();
}

RwTexture *CSnapShots::CreateTextureSnapShot(const char *tex) {
    CRGBA color_white;
    color_white.a = 255;
    color_white.b = 255;
    color_white.g = 255;
    color_white.r = 255;
    RwTexture* texture = (RwTexture*) CUtil::LoadTextureFromDB("samp", tex);
    CSprite2d* m_pSprite2d = new CSprite2d();
    m_pSprite2d->m_pTexture = texture;

    m_pRenderTarget->Begin(512, 512, 0x00000000, false);

    m_pSprite2d->Draw(0,0,texture->raster->height, texture->raster->width, &color_white);


    return m_pRenderTarget->End();
}

RwTexture *CSnapShots::CreatePlayerSnapShot(VECTOR *vecRot, float fZoom) {
    FLog("CreatePlayerSnapShot: , %f, %f, %f", vecRot->X, vecRot->Y, vecRot->Z);

    CPlayerPed *pPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();

    float posZ = pPed->m_pPed->GetModelId() == 162 ? 50.15f : 50.05f;
    float posY = fZoom * -2.25f;
    pPed->m_pPed->SetPosn(0.0f, posY, posZ);
    pPed->SetModelIndex(pPed->m_pPed->GetModelId());
    //pPed->m_pPed->SetGravityProcessing(false);
    pPed->m_pPed->SetCollisionChecking(false);

    RwMatrix mat = pPed->m_pPed->GetMatrix().ToRwMatrix();

    CVector axis { 1.0f, 0.0f, 0.0f };
    if (vecRot->X != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->X);
    }
    axis.Set( 0.0f, 1.0f, 0.0f );
    if (vecRot->Y != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->Y);
    }
    axis.Set( 0.0f, 0.0f, 1.0f );
    if (vecRot->Z != 0.0f)
    {
        RwMatrixRotate(&mat, &axis, vecRot->Z);
    }

    pPed->m_pPed->SetMatrix((CMatrix&)mat);



    pPed->m_pPed->Add();

    m_pRenderTarget->Begin(512, 512, 0x00000000, false);
    pPed->ClumpUpdateAnimations(100.0f, 1);
    RenderEntity(pPed->m_pPed);

    pPed->m_pPed->Remove();

    delete pPed;

    return m_pRenderTarget->End();
}
