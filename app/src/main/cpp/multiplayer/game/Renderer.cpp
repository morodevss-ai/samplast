#include "Renderer.h"
#include "vendor/armhook/patch.h"
#include "app/app_light.h"
#include "VisibilityPlugins.h"
#include "game.h"
#include "Models/ModelInfo.h"
#include "Camera.h"
#include "Scene.h"
#include "graphics/CSkyBox.h"

#include "net/netgame.h"
extern CNetGame* pNetGame;

void CRenderer::AddToLodRenderList(CEntityGTA* entity, float distance) {
    CHook::CallFunction<void>("_ZN9CRenderer18AddToLodRenderListEP7CEntityf", entity, distance);
}

void CRenderer::AddEntityToRenderList(CEntityGTA* entity, float fDistance) {
    CHook::CallFunction<void>("_ZN9CRenderer21AddEntityToRenderListEP7CEntityf", entity, fDistance);
}

int32 CRenderer::SetupMapEntityVisibility(CEntityGTA* entity, CBaseModelInfo* baseModelInfo, float fDistance, bool bIsTimeInRange) {
    if (!entity->m_bTunnelTransition) {
        if (!ms_bRenderTunnels && entity->m_bTunnel || (!ms_bRenderOutsideTunnels && !entity->m_bTunnel)) {
            return RENDERER_INVISIBLE;
        }
    }

    const float fObjectRadius = baseModelInfo->GetColModel()->GetBoundRadius();
    const float fFarClipRadius = fObjectRadius + ms_fFarClipPlane;
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    float fDrawDistanceRadius = std::min(TheCamera.m_fLODDistMultiplier * baseModelInfo->m_fDrawDistance, fFarClipRadius);

    if (!entity->m_pLod) {
        fDrawDistanceRadius *= mobileDrawDistance;
        fDrawDistanceRadius = std::max(fDrawDistanceRadius, 150.0f);
    }

    float fFadingDistance = MAX_FADING_DISTANCE;
    if (!entity->m_pLod) {
        float fDrawDistance = std::max(baseModelInfo->m_fDrawDistance * mobileDrawDistance, fDrawDistanceRadius);
        if (fDrawDistance > MAX_LOWLOD_DISTANCE) {
            fFadingDistance = fDrawDistance / 15.0f + 10.0f;
        }
        if (entity->m_bIsBIGBuilding) {
            fDrawDistanceRadius *= ms_lowLodDistScale;
        }
    }

    if (!baseModelInfo->m_pRwObject) {
        if (entity->m_pLod && entity->m_pLod->m_nNumLodChildren > 1u &&
            fFadingDistance + fDistance - MAX_FADING_DISTANCE < fDrawDistanceRadius) {
            AddToLodRenderList(entity, fDistance);
            return RENDERER_STREAMME;
        }
    }

    if (!baseModelInfo->m_pRwObject || (fFadingDistance + fDistance - MAX_FADING_DISTANCE >= fDrawDistanceRadius)) {
        if (entity->m_bDontStream) {
            return RENDERER_INVISIBLE;
        }
        if (baseModelInfo->m_pRwObject && fDistance - MAX_FADING_DISTANCE < fDrawDistanceRadius) {
            if (!entity->m_pRwObject) {
                entity->CreateRwObject();
                if (!entity->m_pRwObject) {
                    return RENDERER_INVISIBLE;
                }
            }
            if (!entity->m_bIsVisible) {
                return RENDERER_INVISIBLE;
            }
            if (!entity->GetIsOnScreen() || entity->IsEntityOccluded()) {
                if (!baseModelInfo->HasBeenPreRendered()) {
                    baseModelInfo->m_nAlpha = 255;
                }
                baseModelInfo->SetHasBeenPreRendered(false);
                return RENDERER_INVISIBLE;
            }
            entity->m_bDistanceFade = true;
            if (entity->m_pLod && entity->m_pLod->m_nNumLodChildren > 1u) {
                AddToLodRenderList(entity, fDistance);
            }
            else {
                AddEntityToRenderList(entity, fDistance);
            }
            return RENDERER_INVISIBLE;
        }
        if (fDistance - MAX_STREAMING_DISTANCE >= fDrawDistanceRadius || !bIsTimeInRange || !entity->m_bIsVisible)
            return RENDERER_INVISIBLE;

        if (!entity->m_pRwObject)
            entity->CreateRwObject();
        return RENDERER_STREAMME;
    }

    if (!entity->m_pRwObject) {
        entity->CreateRwObject();
        if (!entity->m_pRwObject)
            return RENDERER_INVISIBLE;
    }

    if (!entity->m_bIsVisible)
        return RENDERER_INVISIBLE;

    if (entity->GetIsOnScreen() && !entity->IsEntityOccluded()) {

        if (baseModelInfo->m_nAlpha == 255)
            entity->m_bDistanceFade = false;
        else
            entity->m_bDistanceFade = true;

        if (!entity->m_pLod)
            return RENDERER_VISIBLE;

        if (baseModelInfo->m_nAlpha == 255)
            entity->m_pLod->m_nNumLodChildrenRendered++;

        if (entity->m_pLod->m_nNumLodChildren <= 1u)
            return RENDERER_VISIBLE;

        AddToLodRenderList(entity, fDistance);
        return RENDERER_INVISIBLE;
    }
    if (!baseModelInfo->HasBeenPreRendered())
        baseModelInfo->m_nAlpha = 255;

    baseModelInfo->SetHasBeenPreRendered(false);
    return RENDERER_CULLED;
}

void CRenderer::RenderEverythingBarRoads() {
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));

    RwRenderStateSet(rwRENDERSTATEFOGENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATECULLMODE,          RWRSTATE(rwCULLMODECULLBACK));
    if (!CGame::currArea)
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(140u));

    // CSkyBox::Process();

    assert(ms_nNoOfVisibleEntities <= MAX_VISIBLE_ENTITY_PTRS);
    for (auto& entity : GetVisibleEntityPtrs()) {
        auto* vehicle = entity->AsVehicle();
        if (entity->IsBuilding() && CModelInfo::GetModelInfo(entity->m_nModelIndex)->bIsRoad)
            continue;

        bool bInserted = false;
        if (entity->IsVehicle() || (entity->IsPed() && CVisibilityPlugins::GetClumpAlpha(entity->m_pRwClump) != 255)) {
            // todo: R* nice check | or we missed smth here?
            if (entity->IsVehicle()) {
                bool bInsertIntoSortedList = false;
                if (vehicle->IsBoat()) {
                    const auto& camMode = CCamera::GetActiveCamera().m_nMode;
                    const auto& lookDirection = TheCamera.GetLookDirection();
                    if (camMode == MODE_WHEELCAM || camMode == MODE_1STPERSON
                                                    && lookDirection != LOOKING_DIRECTION_FORWARD
                                                    && lookDirection != LOOKING_DIRECTION_UNKNOWN_1 ||
                        CVisibilityPlugins::GetClumpAlpha(entity->m_pRwClump) != 255) {
                        bInsertIntoSortedList = true;
                    }
                }
                else if (!vehicle->physicalFlags.bTouchingWater) {
                    bInsertIntoSortedList = true;
                }
                const float fMagnitude = DistanceBetweenPoints(entity->GetPosition(), ms_vecCameraPosition);
                if (bInsertIntoSortedList)
                    bInserted = CVisibilityPlugins::InsertEntityIntoSortedList(entity, fMagnitude);
                else
                    bInserted = CVisibilityPlugins::InsertEntityIntoUnderwaterEntities(entity, fMagnitude);
            }
        }
        if (!bInserted)
            RenderOneNonRoad(entity);
    }
    float oldzShift = Scene.m_pRwCamera->zShift;
    RwCameraEndUpdate(Scene.m_pRwCamera);

    Scene.m_pRwCamera->zShift -= 100.0f;
    RwCameraBeginUpdate(Scene.m_pRwCamera);
    for (auto& entity : GetVisibleLodPtrs()) {
        RenderOneNonRoad(entity);
    }
    RwCameraEndUpdate(Scene.m_pRwCamera);

    Scene.m_pRwCamera->zShift = oldzShift;
    RwCameraBeginUpdate(Scene.m_pRwCamera);

    if (pNetGame) {
        CObjectPool* pObjectPool = pNetGame->GetObjectPool();
        if (pObjectPool) {
            for (OBJECTID i = 0; i < MAX_OBJECTS; i++) {
                CObject* pObject = pObjectPool->GetAt(i);
                if (pObject && pObject->m_bForceRender) {
                    // CEntity::PreRender
                    ((void (*)(CEntityGTA*))(*(void**)(pObject->m_pEntity + (VER_x32 ? 0x48:0x48*2))))(pObject->m_pEntity);

                    // CRenderer::RenderOneNonRoad
                    ((void (*)(CEntityGTA*))(g_libGTASA+ (VER_x32 ? 0x41030C + 1:0x4F56E0)))(pObject->m_pEntity);
                }
            }
        }
    }
}

void CRenderer::RenderOneNonRoad(CEntityGTA* entity) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0041030C + 1 : 0x4F56E0), entity);
}

void CRenderer::RenderFadingInUnderwaterEntities() {
    CHook::CallFunction<void>("_ZN9CRenderer32RenderFadingInUnderwaterEntitiesEv");
}

void CRenderer::RenderRoads() {
    CHook::CallFunction<void>("_ZN9CRenderer11RenderRoadsEv");
}

void CRenderer::RenderFadingInEntities() {
    RwRenderStateSet(rwRENDERSTATEFOGENABLE,         RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, RWRSTATE(TRUE));
    RwRenderStateSet(rwRENDERSTATECULLMODE,          RWRSTATE(rwCULLMODECULLBACK));
    DeActivateDirectional();
    SetAmbientColours();
    CVisibilityPlugins::RenderFadingEntities();
}

void CRenderer::UpdateLodDistScale(int value) {
    CRenderer::ms_lodDistScale = static_cast<float>((static_cast<float>(value * 0.875) * 0.01) + 0.875);
}

void CRenderer::InjectHooks()
{
    CHook::Redirect("_ZN9CRenderer24RenderEverythingBarRoadsEv", &RenderEverythingBarRoads);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x6764D0 : 0x84AA10), &ms_bRenderOutsideTunnels);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x67914C : 0x8502C8), &m_loadingPriority);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x678BEC : 0x84F808), &ms_vecCameraPosition);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x6778EC : 0x84D210), &ms_aVisibleEntityPtrs);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6771F0 : 0x84C428), &ms_nNoOfVisibleEntities);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x676474 : 0x84A958), &ms_aVisibleLodPtrs);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x676210 : 0x84A4A0), &ms_nNoOfVisibleLods);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x679B4C : 0x8516B8), &ms_lodDistScale);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x677594 : 0x84CB78), &ms_fFarClipPlane);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x67783C : 0x84D0B0), &ms_lowLodDistScale);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6789C4 : 0x84F3B0), &ms_bRenderTunnels);
}

void CRenderer::ConstructRenderList() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x40F6FC + 1 : 0x4F4A48));
}

void CRenderer::PreRender() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x40F990 + 1 : 0x4F4CCC));
}

void CRenderer::ScanWorld() {

}