#include "VisibilityPlugins.h"
#include "../vendor/armhook/patch.h"
#include "constants.h"
#include "Camera.h"
#include "game.h"
#include "Renderer.h"
#include "Models/ModelInfo.h"

void CVisibilityPlugins::SetAtomicFlag(RpAtomic* atomic, uint16 flag) {
    CHook::CallFunction<void>("_ZN18CVisibilityPlugins13SetAtomicFlagEP8RpAtomict", atomic, flag);
}

void CVisibilityPlugins::ClearAtomicFlag(RpAtomic *atomic, uint16 flag) {
    CHook::CallFunction<void>("_ZN18CVisibilityPlugins15ClearAtomicFlagEP8RpAtomict", atomic, flag);
}

RpAtomic* CVisibilityPlugins::RenderVehicleHiDetailCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_vehicleLod0Dist) {
        return atomic;
    }

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    const uint16 atomicFlags = GetAtomicId(atomic);
    if (gVehicleDistanceFromCamera > ms_cullCompsDist && !(atomicFlags & ATOMIC_RENDER_ALWAYS)) {
        if ((float)(g_libGTASA + (VER_x32 ? 0xA84174 : 0xD28A00)) < 0.2f) {
            RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
            RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
            const float dot = GetDotProductWithCameraVector(atomicMatrix, clumpMatrix, atomicFlags);
            if (dot > 0.0f && ((atomicFlags & ATOMIC_CULL) || gVehicleDistanceFromCamera * 0.1f < dot * dot))
                return atomic;
        }
    }

    AtomicDefaultRenderCallBack(atomic);
    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleReallyLowDetailCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera < ms_vehicleLod0Dist) {
        return atomic;
    }

    SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    if (GetClumpAlpha(RpAtomicGetClump(atomic)) == 255) {
        AtomicDefaultRenderCallBack(atomic);
    }
    else {
        RenderAlphaAtomic(atomic, GetClumpAlpha(RpAtomicGetClump(atomic)));
    }
    return atomic;
}

RpAtomic* CVisibilityPlugins::DefaultAtomicRenderCallback(RpAtomic* atomic)
{
    return AtomicDefaultRenderCallBack(atomic);
}

RpAtomic* CVisibilityPlugins::RenderVehicleHiDetailAlphaCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_vehicleLod0Dist) {
        return atomic;
    }

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    const uint16 atomicFlags = GetAtomicId(atomic);
    RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
    const float dot = GetDotProductWithCameraVector(atomicMatrix, clumpMatrix, atomicFlags);
    if (gVehicleDistanceFromCamera > ms_cullCompsDist
        && !(atomicFlags & ATOMIC_RENDER_ALWAYS)
        && (float)(g_libGTASA + (VER_x32 ? 0xA84174 : 0xD28A00)) < 0.2f
        && dot > 0.0f
        && ((atomicFlags & ATOMIC_CULL) || gVehicleDistanceFromCamera * 0.1f < dot * dot))
    {
        return atomic;
    }
    AlphaObjectInfo info{};
    info.m_pCallback = reinterpret_cast<void*>(DefaultAtomicRenderCallback);
    info.m_atomic = atomic;
    if (atomicFlags & ATOMIC_HAS_ALPHA)
        info.m_distance = gVehicleDistanceFromCamera - 0.0001f;
    else
        info.m_distance = gVehicleDistanceFromCamera + dot;
    if (!m_alphaList.InsertSorted(info))
        AtomicDefaultRenderCallBack(atomic);

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderHeliRotorAlphaCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_bigVehicleLod0Dist)
        return atomic;

    RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
    CVector distance = atomicMatrix->pos - *ms_pCameraPosn;
    const float dotProduct = distance.Dot(clumpMatrix->at);
    AlphaObjectInfo objectInfo{};
    objectInfo.m_atomic = atomic;
    objectInfo.m_pCallback = reinterpret_cast<void*>(DefaultAtomicRenderCallback);
    objectInfo.m_distance = dotProduct * MAX_FADING_DISTANCE + gVehicleDistanceFromCamera;
    if (!m_alphaList.InsertSorted(objectInfo)) {
        AtomicDefaultRenderCallBack(atomic);
    }
    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderHeliTailRotorAlphaCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_vehicleLod0Dist)
        return atomic;

    RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
    CVector distance = atomicMatrix->pos - *ms_pCameraPosn;
    const float dotProduct1 = distance.Dot(clumpMatrix->right);
    const float dotProduct2 = distance.Dot(clumpMatrix->up);
    AlphaObjectInfo info{};
    info.m_atomic = atomic;
    info.m_pCallback = reinterpret_cast<void*>(DefaultAtomicRenderCallback);
    info.m_distance = -dotProduct1 - dotProduct2 + gVehicleDistanceFromCamera;
    if (!m_alphaList.InsertSorted(info))
        AtomicDefaultRenderCallBack(atomic);
    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleReallyLowDetailCB_BigVehicle(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_bigVehicleLod0Dist)
        AtomicDefaultRenderCallBack(atomic);

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_BigVehicle(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_bigVehicleLod0Dist)
        return atomic;

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    const uint16 atomicFlags = GetAtomicId(atomic);
    RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
    const float dot = GetDotProductWithCameraVector(atomicMatrix, clumpMatrix, atomicFlags);
    if (gVehicleDistanceFromCamera > ms_cullBigCompsDist
        && !(atomicFlags & ATOMIC_RENDER_ALWAYS)
        && (float)(g_libGTASA + (VER_x32 ? 0xA84174 : 0xD28A00)) < 0.2f
        && dot > 0.0f
        && ((atomicFlags & ATOMIC_CULL) || gVehicleDistanceFromCamera * 0.1f < dot * dot))
    {
        return atomic;
    }
    AlphaObjectInfo info{};
    info.m_pCallback = reinterpret_cast<void*>(DefaultAtomicRenderCallback);
    info.m_atomic = atomic;
    if (atomicFlags & ATOMIC_HAS_ALPHA)
        info.m_distance = gVehicleDistanceFromCamera - 0.0001f;
    else
        info.m_distance = gVehicleDistanceFromCamera + dot;
    if (!m_alphaList.InsertSorted(info))
        AtomicDefaultRenderCallBack(atomic);

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleHiDetailCB_BigVehicle(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_bigVehicleLod0Dist)
        return atomic;

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    const uint16 atomicFlags = GetAtomicId(atomic);
    if (gVehicleDistanceFromCamera > ms_cullBigCompsDist && !(atomicFlags & ATOMIC_RENDER_ALWAYS)) {
        if ((float)(g_libGTASA + (VER_x32 ? 0xA84174 : 0xD28A00)) < 0.2f) {
            RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
            RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
            const float dot = GetDotProductWithCameraVector(atomicMatrix, clumpMatrix, atomicFlags);
            if (dot > 0.0f)
                return atomic;
        }
    }
    AtomicDefaultRenderCallBack(atomic);

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleHiDetailCB_Boat(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_vehicleLod0Dist)
        return atomic;

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    AtomicDefaultRenderCallBack(atomic);

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleLoDetailCB_Boat(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera < ms_vehicleLod0Dist)
        return atomic;

    SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    if (GetClumpAlpha(RpAtomicGetClump(atomic)) == 255)
        AtomicDefaultRenderCallBack(atomic);
    else
        RenderAlphaAtomic(atomic, GetClumpAlpha(RpAtomicGetClump(atomic)));

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderVehicleHiDetailAlphaCB_Boat(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_vehicleLod0Dist)
        return atomic;

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    const uint16 atomicFlags = GetAtomicId(atomic);
    if (atomicFlags & ATOMIC_HAS_ALPHA) {
        AlphaObjectInfo info{};
        info.m_distance = gVehicleDistanceFromCamera;
        info.m_atomic = atomic;
        info.m_pCallback = reinterpret_cast<void*>(DefaultAtomicRenderCallback);
        if (m_alphaBoatAtomicList.InsertSorted(info))
            return atomic;
    }
    AtomicDefaultRenderCallBack(atomic);
    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderTrainHiDetailAlphaCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_bigVehicleLod0Dist)
        return atomic;

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    const uint16 atomicFlags = GetAtomicId(atomic);
    RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
    const float dot = GetDotProductWithCameraVector(atomicMatrix, clumpMatrix, atomicFlags);
    if (gVehicleDistanceFromCamera > ms_cullCompsDist
        && !(atomicFlags & ATOMIC_RENDER_ALWAYS)
        && (float)(g_libGTASA + (VER_x32 ? 0xA84174 : 0xD28A00)) < 0.2f
        && dot > 0.0f
        && ((atomicFlags & ATOMIC_CULL) || gVehicleDistanceFromCamera * 0.1f < dot * dot))
    {
        return atomic;
    }
    AlphaObjectInfo info{};
    info.m_pCallback = reinterpret_cast<void*>(DefaultAtomicRenderCallback);
    info.m_atomic = atomic;
    if (atomicFlags & ATOMIC_HAS_ALPHA)
        info.m_distance = gVehicleDistanceFromCamera;
    else
        info.m_distance = gVehicleDistanceFromCamera + dot;
    if (!m_alphaList.InsertSorted(info))
        AtomicDefaultRenderCallBack(atomic);

    return atomic;
}

RpAtomic* CVisibilityPlugins::RenderTrainHiDetailCB(RpAtomic* atomic) {
    if (gVehicleDistanceFromCamera >= ms_bigVehicleLod0Dist)
        return atomic;

    if (gVehicleDistanceFromCamera >= ms_vehicleLod0RenderMultiPassDist)
        SetAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);
    else
        ClearAtomicFlag(atomic, ATOMIC_DISABLE_REFLECTIONS);

    const uint16 atomicFlags = GetAtomicId(atomic);
    if (gVehicleDistanceFromCamera > ms_cullCompsDist && !(atomicFlags & ATOMIC_RENDER_ALWAYS)) {
        if ((float)(g_libGTASA + (VER_x32 ? 0xA84174 : 0xD28A00)) < 0.2f) {
            RwMatrix* atomicMatrix = RwFrameGetLTM(RpAtomicGetFrame(atomic));
            RwMatrix* clumpMatrix = RwFrameGetLTM(RpClumpGetFrame(RpAtomicGetClump(atomic)));
            const float dot = GetDotProductWithCameraVector(atomicMatrix, clumpMatrix, atomicFlags);
            if (dot > 0.0f && ((atomicFlags & ATOMIC_CULL) || gVehicleDistanceFromCamera * 0.1f < dot * dot))
                return atomic;
        }
    }
    AtomicDefaultRenderCallBack(atomic);
    return atomic;
}

bool CVisibilityPlugins::InsertEntityIntoUnderwaterEntities(CEntityGTA* entity, float distance)
{
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x5D6B58 + 1 : 0x6FBFDC), entity, distance);
}

bool CVisibilityPlugins::InsertEntityIntoSortedList(CEntityGTA* entity, float distance) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x5D6758 + 1 : 0x6FBB4C), entity, distance);
}

void CVisibilityPlugins::Initialise() {
    m_alphaList.Init(TOTAL_ALPHA_LISTS);
    m_alphaList.usedListHead.data.m_distance = 0.0f;
    m_alphaList.usedListTail.data.m_distance = 100000000.0f;

    m_alphaBoatAtomicList.Init(TOTAL_ALPHA_BOAT_ATOMIC_LISTS);
    m_alphaBoatAtomicList.usedListHead.data.m_distance = 0.0f;
    m_alphaBoatAtomicList.usedListTail.data.m_distance = 100000000.0f;

    m_alphaEntityList.Init(TOTAL_ALPHA_ENTITY_LISTS);
    m_alphaEntityList.usedListHead.data.m_distance = 0.0f;
    m_alphaEntityList.usedListTail.data.m_distance = 100000000.0f;

    m_alphaUnderwaterEntityList.Init(TOTAL_ALPHA_UNDERWATER_ENTITY_LISTS);
    m_alphaUnderwaterEntityList.usedListHead.data.m_distance = 0.0f;
    m_alphaUnderwaterEntityList.usedListTail.data.m_distance = 100000000.0f;

    // New in SA
    m_alphaReallyDrawLastList.Init(TOTAL_ALPHA_DRAW_LAST_LISTS);
    m_alphaReallyDrawLastList.usedListHead.data.m_distance = 0.0f;
    m_alphaReallyDrawLastList.usedListTail.data.m_distance = 100000000.0f;

    ms_weaponPedsForPC.Init(TOTAL_WEAPON_PEDS_FOR_PC);

    //CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D446C + 1 : 0x6F954C));
}

void CVisibilityPlugins::RenderFadingEntities() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x5D6DAC + 1 : 0x6FC284));
}

void CVisibilityPlugins::SetRenderWareCamera(RwCamera* camera) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D6248 + 1 : 0x6FB5BC), camera);
}

void CVisibilityPlugins::SetAtomicRenderCallback(RpAtomic* atomic, RpAtomicCallBackRender renderCB) {
    if (!renderCB) {
        renderCB = AtomicDefaultRenderCallBack;
    }
    atomic->renderCallBack = renderCB;
    if (!renderCB) {
        atomic->renderCallBack = AtomicDefaultRenderCallBack;
    }
}

RpAtomic* CVisibilityPlugins::RenderPedCB(RpAtomic* atomic) {
    const float distanceSquared = GetDistanceSquaredFromCamera(RpAtomicGetFrame(atomic));
    if (distanceSquared >= ms_pedLodDist)
        return atomic;

    int32 alpha = GetClumpAlpha(RpAtomicGetClump(atomic));
    if (alpha == 255) {
        AtomicDefaultRenderCallBack(atomic);
        return atomic;
    }
    RenderAlphaAtomic(atomic, alpha);
    return atomic;
}

float CVisibilityPlugins::GetDistanceSquaredFromCamera(RwFrame* frame) {
    RwMatrix* transformMatrix = RwFrameGetLTM(frame);
    CVector distance;
    RwV3dSub(&distance, &transformMatrix->pos, ms_pCameraPosn);
    return distance.SquaredMagnitude();
}

int32 CVisibilityPlugins::GetClumpAlpha(RpClump* clump) {
    return CHook::CallFunction<int32>(g_libGTASA + (VER_x32 ? 0x005D4FEC + 1 : 0x6FA290), clump);
}

void CVisibilityPlugins::SetClumpAlpha(RpClump* clump, int32 alpha) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x5D65E4 + 1 : 0x6FB9C0), clump, alpha);
}

#include "rpworld.h"
void CVisibilityPlugins::RenderAlphaAtomic(RpAtomic* atomic, int32 alpha) {
    uint8 alphas[256];

    RpGeometry* geometry = RpAtomicGetGeometry(atomic);
    uint32 geometryFlags = RpGeometryGetFlags(geometry);
    RpGeometrySetFlags(geometry, geometryFlags | rpGEOMETRYMODULATEMATERIALCOLOR);

    const int32 numMaterials = RpGeometryGetNumMaterials(geometry);
    for (int32 i = 0; i < numMaterials; i++) {
        RpMaterial* material = RpGeometryGetMaterial(geometry, i);
        RwRGBA* color        = RpMaterialGetColor(material);
        alphas[i]            = color->alpha;
        color->alpha         = std::min(color->alpha, (uint8)alpha);
    }

    AtomicDefaultRenderCallBack(atomic);

    for (int32 i = 0; i < numMaterials; i++) {
        RpMaterial* material = RpGeometryGetMaterial(geometry, i);
        RwRGBA* color        = RpMaterialGetColor(material);
        color->alpha         = alphas[i];
    }
    RpGeometrySetFlags(geometry, geometryFlags);
}

void CVisibilityPlugins::RenderEntity(CEntityGTA* entity, float distance) {
    if (!entity->m_pRwObject)
        return;

    CBaseModelInfo* mi = CModelInfo::GetModelInfo(entity->m_nModelIndex);
    if (mi->m_nAlpha < 0)
        return;

    const bool bDontWriteZ = mi->bDontWriteZBuffer;
    if (bDontWriteZ)
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(FALSE));

    if (!entity->m_bDistanceFade) {
        if (CGame::currArea || mi->bDontWriteZBuffer)
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(0));
        else
            RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(100u));
        CRenderer::RenderOneNonRoad(entity);
    }
    else {
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, RWRSTATE(0));
        int32 alpha = CalculateFadingAtomicAlpha(mi, entity, distance);
        if (alpha <= 0) {
            if (bDontWriteZ)
                RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
            return;
        }

        entity->m_bImBeingRendered = true;

        const bool bBackfaceCulled = entity->m_bBackfaceCulled;
        if (!bBackfaceCulled)
            RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLNONE));

        bool bLightingSetup = entity->SetupLighting();

        if (RwObjectGetType(entity->m_pRwObject) == rpATOMIC)
            RenderFadingAtomic(mi, entity->m_pRwAtomic, alpha);
        else
            RenderFadingClump(mi, entity->m_pRwClump, alpha);

        entity->RemoveLighting(bLightingSetup);
        entity->m_bImBeingRendered = false;

        if (!bBackfaceCulled)
            RwRenderStateSet(rwRENDERSTATECULLMODE, RWRSTATE(rwCULLMODECULLBACK));
    }

    if (bDontWriteZ)
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(TRUE));
}

int32 CVisibilityPlugins::CalculateFadingAtomicAlpha(CBaseModelInfo* modelInfo, CEntityGTA* entity, float distance) {
    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    float fDrawDistanceRadius = TheCamera.m_fLODDistMultiplier * modelInfo->m_fDrawDistance;

    if (!entity->m_pLod) {
        fDrawDistanceRadius *= CRenderer::mobileDrawDistance;
        fDrawDistanceRadius = std::max(fDrawDistanceRadius, 150.0f);
    }

    fDrawDistanceRadius = std::min(fDrawDistanceRadius,
                                   modelInfo->GetColModel()->GetBoundRadius() + CRenderer::ms_fFarClipPlane);

    float fFadingDistance = MAX_FADING_DISTANCE;
    if (!entity->m_pLod) {
        float fAdjustedDistance = std::min(modelInfo->m_fDrawDistance * CRenderer::mobileDrawDistance,
                                           fDrawDistanceRadius);
        if (fAdjustedDistance > MAX_LOWLOD_DISTANCE) {
            fFadingDistance = fAdjustedDistance / 15.0f + 10.0f;
        }
        if (entity->m_bIsBIGBuilding) {
            fDrawDistanceRadius *= CRenderer::ms_lowLodDistScale;
        }
    }

    float fFade = std::clamp(
            (fDrawDistanceRadius + MAX_FADING_DISTANCE - distance) / fFadingDistance,
            0.0f, 1.0f
    );
    return static_cast<int32>(modelInfo->m_nAlpha * fFade); // Квадратичное затухание для лучшего вида
}

void CVisibilityPlugins::RenderFadingAtomic(CBaseModelInfo* modelInfo, RpAtomic* atomic, int32 alpha) {
    if (modelInfo->bAdditiveRender) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDONE));
    }

    RenderAlphaAtomic(atomic, alpha);

    if (modelInfo->bAdditiveRender) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
    }
}

void CVisibilityPlugins::RenderFadingClump(CBaseModelInfo* modelInfo, RpClump* clump, int32 alpha) {
    if (modelInfo->bAdditiveRender) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDONE));
    }

    RpClumpForAllAtomics(clump, RenderAtomicWithAlphaCB, &alpha);
    if (modelInfo->bAdditiveRender) {
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, RWRSTATE(rwBLENDINVSRCALPHA));
    }
}

void CVisibilityPlugins::InitAlphaAtomicList() {
    CHook::CallFunction<void>("_ZN18CVisibilityPlugins19InitAlphaAtomicListEv");
}

void CVisibilityPlugins::RenderAlphaAtomics() {
    CHook::CallFunction<void>("_ZN18CVisibilityPlugins18RenderAlphaAtomicsEv");
}

RpAtomic* CVisibilityPlugins::RenderAtomicWithAlphaCB(RpAtomic* atomic, void* data) {
    if (RpAtomicGetFlags(atomic) & rpATOMICRENDER)
        RenderAlphaAtomic(atomic, *reinterpret_cast<int32*>(data));
    return atomic;
}

void CVisibilityPlugins::InjectHooks() {
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00678E3C : 0x84FCA8), &ms_cullCompsDist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x0067905C : 0x8500E8), &ms_cullBigCompsDist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x006791E4 : 0x8503F8), &ms_vehicleLod0RenderMultiPassDist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00677648 : 0x84CCD0), &ms_vehicleLod0Dist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00676DA4 : 0x84BBA8), &ms_vehicleLod1Dist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00675FF0 : 0x84A068), &ms_bigVehicleLod0Dist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00677814 : 0x84D060), &gVehicleDistanceFromCamera);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x676444 : 0x84A900), &m_alphaList);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x677D60 : 0x84DAF0), &m_alphaEntityList);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6768FC : 0x84B260), &m_alphaBoatAtomicList);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x675F84 : 0x849F90), &m_alphaUnderwaterEntityList);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x6799D4 : 0x8513C8), &m_alphaReallyDrawLastList);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x679CF8 : 0x851A08), &ms_weaponPedsForPC);

    CHook::Write(g_libGTASA + (VER_x32 ? 0x00676200 : 0x84A480), &ms_pedLodDist);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00676530 : 0x84AAD0), &ms_pCameraPosn);

    CHook::Redirect("_ZN18CVisibilityPlugins23RenderVehicleHiDetailCBEP8RpAtomic", &RenderVehicleHiDetailCB);
    CHook::Redirect("_ZN18CVisibilityPlugins30RenderVehicleReallyLowDetailCBEP8RpAtomic", &RenderVehicleReallyLowDetailCB);
    CHook::Redirect("_ZN18CVisibilityPlugins28RenderVehicleHiDetailAlphaCBEP8RpAtomic", &RenderVehicleHiDetailAlphaCB);
    CHook::Redirect("_ZN18CVisibilityPlugins22RenderHeliRotorAlphaCBEP8RpAtomic", &RenderHeliRotorAlphaCB);
    CHook::Redirect("_ZN18CVisibilityPlugins26RenderHeliTailRotorAlphaCBEP8RpAtomic", &RenderHeliTailRotorAlphaCB);
    CHook::Redirect("_ZN18CVisibilityPlugins41RenderVehicleReallyLowDetailCB_BigVehicleEP8RpAtomic", &RenderVehicleReallyLowDetailCB_BigVehicle);
    CHook::Redirect("_ZN18CVisibilityPlugins39RenderVehicleHiDetailAlphaCB_BigVehicleEP8RpAtomic", &RenderVehicleHiDetailAlphaCB_BigVehicle);
    CHook::Redirect("_ZN18CVisibilityPlugins34RenderVehicleHiDetailCB_BigVehicleEP8RpAtomic", &RenderVehicleHiDetailCB_BigVehicle);
    CHook::Redirect("_ZN18CVisibilityPlugins28RenderVehicleHiDetailCB_BoatEP8RpAtomic", &RenderVehicleHiDetailCB_Boat);
    CHook::Redirect("_ZN18CVisibilityPlugins28RenderVehicleLoDetailCB_BoatEP8RpAtomic", &RenderVehicleLoDetailCB_Boat);
    CHook::Redirect("_ZN18CVisibilityPlugins33RenderVehicleHiDetailAlphaCB_BoatEP8RpAtomic", &RenderVehicleHiDetailAlphaCB_Boat);
    CHook::Redirect("_ZN18CVisibilityPlugins26RenderTrainHiDetailAlphaCBEP8RpAtomic", &RenderTrainHiDetailAlphaCB);
    CHook::Redirect("_ZN18CVisibilityPlugins21RenderTrainHiDetailCBEP8RpAtomic", &RenderTrainHiDetailCB);

    //CHook::Redirect("_ZN18CVisibilityPlugins12RenderEntityEPvf", &RenderEntity);
}

void CVisibilityPlugins::SetupVehicleVariables(RpClump *clump) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D4B90 + 1 : 0x6F9DB0), clump);
}

void CVisibilityPlugins::RenderReallyDrawLastObjects() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x005D6EC4 + 1 : 0x6FC3DC));
}

// The function name is misleading, it returns the flags
uint16 CVisibilityPlugins::GetAtomicId(RpAtomic* atomic) {
    return CHook::CallFunction<uint16>(g_libGTASA + (VER_x32 ? 0x5D4B54 + 1 : 0x6F9D68), atomic);
}

float CVisibilityPlugins::GetDotProductWithCameraVector(RwMatrix* atomicMatrix, RwMatrix* clumpMatrix, uint16 flags)
{
    const CVector distance = atomicMatrix->pos - *ms_pCameraPosn;

    float dotProduct1 = 0.0f;
    float dotProduct2 = 0.0f;

    if (flags & (ATOMIC_IS_REAR | ATOMIC_IS_FRONT)) {
        dotProduct1 = DotProduct(clumpMatrix->up, &distance);
    }
    else if (flags & (ATOMIC_IS_RIGHT | ATOMIC_IS_LEFT)) {
        dotProduct1 = DotProduct(clumpMatrix->right, &distance);
    }

    if (flags & (ATOMIC_IS_REAR | ATOMIC_IS_LEFT)) {
        dotProduct1 = -dotProduct1;
    }

    if (flags & ATOMIC_VEHCOMP_15) {
        const float dot = DotProduct(clumpMatrix->at, &distance);
        const float multiplier = (flags & (ATOMIC_IS_FRONT_DOOR | ATOMIC_IS_REAR_DOOR)) ? 0.25f : 2.5f;
        dotProduct1 += dot * multiplier;
    }

    if (!(flags & (ATOMIC_IS_FRONT_DOOR | ATOMIC_IS_REAR_DOOR))) {
        return dotProduct1;
    }

    if (flags & ATOMIC_IS_REAR_DOOR) {
        dotProduct2 = -DotProduct(clumpMatrix->up, &distance);
    }
    else if (flags & ATOMIC_IS_FRONT_DOOR) {
        dotProduct2 = DotProduct(clumpMatrix->up, &distance);
    }

    if ((dotProduct1 < 0.0f && dotProduct2 < 0.0f) ||
        (dotProduct1 > 0.0f && dotProduct2 > 0.0f)) {
        return dotProduct1 + dotProduct2;
    }

    return dotProduct1;
}

void CVisibilityPlugins::RenderWeaponPedsForPC() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x5D487C + 1 : 0x6F99F4));
}




