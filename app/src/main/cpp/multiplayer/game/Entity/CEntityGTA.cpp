//
// Created by roman on 11/24/2024.
//

#include "CEntityGTA.h"
#include "game/game.h"
#include "game/Collision/ColStore.h"
#include "patch.h"
#include "game/General.h"
#include "game/Models/BaseModelInfo.h"
#include "game/Models/ModelInfo.h"
#include "game/References.h"
#include "game/Pools.h"

extern CGame* pGame;

CEntityGTA::CEntityGTA() : CPlaceable() {
    m_nStatus = STATUS_ABANDONED;
    m_nType = ENTITY_TYPE_NOTHING;

    m_nFlags = 0;
    m_bIsVisible = true;
    m_bBackfaceCulled = true;

    m_nScanCode = 0;
    m_nAreaCode = eAreaCodes::AREA_CODE_NORMAL_WORLD;
    m_nModelIndex = 0xFFFF;
    m_pRwObject = nullptr;
    m_nIplIndex = 0;
    m_nRandomSeed = CGeneral::GetRandomNumber();
    m_pReferences = nullptr;
    m_pStreamingLink = nullptr;
    m_nNumLodChildren = 0;
    m_nNumLodChildrenRendered = 0;
    m_pLod = nullptr;
}

CEntityGTA::~CEntityGTA()
{
    if (m_pLod)
        m_pLod->m_nNumLodChildren--;

    CEntityGTA::DeleteRwObject();
    CEntityGTA::ResolveReferences();
}

void CEntityGTA::ResolveReferences()
{
    auto refs = m_pReferences;
    while (refs) {
        if (*refs->m_ppEntity == this)
            *refs->m_ppEntity = nullptr;

        refs = refs->m_pNext;
    }

    refs = m_pReferences;
    if (!refs)
        return;

    refs->m_ppEntity = nullptr;
    while (refs->m_pNext)
        refs = refs->m_pNext;

    refs->m_pNext = CReferences::pEmptyList;
    CReferences::pEmptyList = m_pReferences;
    m_pReferences = nullptr;
}

void CEntityGTA::Add()
{
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x3ED8D8 + 1 : 0x4CD574), this);
}

void CEntityGTA::SetInterior(int interiorId, bool needRefresh)
{
    m_nAreaCode = static_cast<eAreaCodes>(interiorId);

    if ( this == (CEntityGTA*)pGame->FindPlayerPed()->m_pPed )
    {
        CGame::currArea = interiorId;

        auto pos = GetPosition();
        CColStore::RequestCollision(&pos, m_nAreaCode);

        if(interiorId == 0) {
            CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x00420898 + 1 : 0x504194), false);
            // CTimeCycle::StopExtraColour(0);
        }

    }

}

float CEntityGTA::GetDistanceFromLocalPlayerPed() const
{
    auto pLocalPlayerPed = pGame->FindPlayerPed();

    return DistanceBetweenPoints(GetPosition(), pLocalPlayerPed->m_pPed->GetPosition());
}

float CEntityGTA::GetDistanceFromCamera()
{
    if(!this)
        return 0;

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    return DistanceBetweenPoints(GetPosition(), TheCamera.GetPosition());
}

float CEntityGTA::GetDistanceFromPoint(float X, float Y, float Z) const
{
    CVector vec(X, Y, Z);

    return DistanceBetweenPoints(GetPosition(), vec);
}

void CEntityGTA::SetCollisionChecking(bool bCheck)
{
    m_bCollisionProcessed = bCheck;
}

void CEntityGTA::UpdateRpHAnim() {
    if(m_pRwClump) {
        if (const auto atomic = GetFirstAtomic(m_pRwClump)) {
            if (RpSkinGeometryGetSkin(RpAtomicGetGeometry(atomic)) && !m_bDontUpdateHierarchy) {
                RpHAnimHierarchyUpdateMatrices(GetAnimHierarchyFromSkinClump(m_pRwClump));
            }
        }
    }
}

void CEntityGTA::UpdateRwFrame()
{
    if (!m_pRwObject)
        return;

    RwFrameUpdateObjects(static_cast<RwFrame*>(rwObjectGetParent(m_pRwObject)));
}

/*void CEntityGTA::DeleteRwObject()
{
    if(!*(uintptr*)this) return;

    (( void (*)(CEntityGTA*))(*(void**)(*(uintptr*)this + (VER_x32 ? 0x24 : 0x24*2))))(this);
}*/

void CEntityGTA::UpdateRW() {
    if (!m_pRwObject)
        return;

    auto parentMatrix = GetModellingMatrix();
    if (m_matrix)
        m_matrix->UpdateRwMatrix(parentMatrix);
    else
        m_placement.UpdateRwMatrix(parentMatrix);
}


RwMatrix* CEntityGTA::GetModellingMatrix() {
    if (!m_pRwObject)
        return nullptr;

    return RwFrameGetMatrix(RwFrameGetParent(m_pRwObject));
}

CBaseModelInfo* CEntityGTA::GetModelInfo() const {
    return CModelInfo::GetModelInfo(m_nModelIndex);
}

void CEntityGTA::Add(const CRect* rect) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x3ED8FC + 1 : 0x4CD5C0), this, rect);
}

void CEntityGTA::Remove() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x3EDBE8 + 1 : 0x4CD888), this);
}

void CEntityGTA::SetModelIndex(uint32 index) {
}

void CEntityGTA::SetModelIndexNoCreate(uint32 index) {
}

void CEntityGTA::SetIsStatic(bool isStatic) {

}

void CEntityGTA::RegisterReference(CEntityGTA** entity)
{
    if (IsBuilding() && !m_bIsTempBuilding && !m_bIsProcObject && !m_nIplIndex)
        return;

    auto refs = m_pReferences;
    while (refs) {
        if (refs->m_ppEntity == entity) {
            return;
        }
        refs = refs->m_pNext;
    }

    if (!m_pReferences && !CReferences::pEmptyList) {
        auto iPedsSize = GetPedPoolGta()->GetSize();
        for (int32 i = 0; i < iPedsSize; ++i) {
            auto ped = GetPedPoolGta()->GetAt(i);
            if (ped) {
                ped->PruneReferences();
                if (CReferences::pEmptyList)
                    break;
            }

        }

        if (!CReferences::pEmptyList) {
            auto iVehsSize = GetVehiclePoolGta()->GetSize();
            for (int32 i = 0; i < iVehsSize; ++i) {
                auto vehicle = GetVehiclePoolGta()->GetAt(i);
                if (vehicle) {
                    vehicle->PruneReferences();
                    if (CReferences::pEmptyList)
                        break;
                }

            }
        }

        if (!CReferences::pEmptyList) {
            auto iObjectsSize = GetObjectPoolGta()->GetSize();
            for (int32 i = 0; i < iObjectsSize; ++i) {
                auto obj = GetObjectPoolGta()->GetAt(i);
                if (obj) {
                    obj->PruneReferences();
                    if (CReferences::pEmptyList)
                        break;
                }
            }
        }
    }

    if (CReferences::pEmptyList) {
        auto pEmptyRef = CReferences::pEmptyList;
        CReferences::pEmptyList = pEmptyRef->m_pNext;
        pEmptyRef->m_pNext = m_pReferences;
        m_pReferences = pEmptyRef;
        pEmptyRef->m_ppEntity = entity;
    }
}

void CEntityGTA::PruneReferences()
{
    if (!m_pReferences)
        return;

    auto refs = m_pReferences;
    auto ppPrev = &m_pReferences;
    while (refs) {
        if (*refs->m_ppEntity == this) {
            ppPrev = &refs->m_pNext;
            refs = refs->m_pNext;
        }
        else {
            auto refTemp = refs->m_pNext;
            *ppPrev = refs->m_pNext;
            refs->m_pNext = CReferences::pEmptyList;
            CReferences::pEmptyList = refs;
            refs->m_ppEntity = nullptr;
            refs = refTemp;
        }
    }
}

void CEntityGTA::CleanUpOldReference(CEntityGTA** entity)
{
    if (!m_pReferences)
        return;

    auto refs = m_pReferences;
    auto ppPrev = &m_pReferences;
    while (refs->m_ppEntity != entity) {
        ppPrev = &refs->m_pNext;
        refs = refs->m_pNext;
        if (!refs)
            return;
    }

    *ppPrev = refs->m_pNext;
    refs->m_pNext = CReferences::pEmptyList;
    refs->m_ppEntity = nullptr;
    CReferences::pEmptyList = refs;
}

CVector* CEntityGTA::GetBoundCentre(CVector* pOutCentre) {
    auto mi = CModelInfo::GetModelInfo(m_nModelIndex);
    const auto& colCenter = mi->GetColModel()->GetBoundCenter();

    return TransformFromObjectSpace(*pOutCentre, colCenter);
}

#include "CUtil.h"
CVector CEntityGTA::TransformFromObjectSpace(const CVector& offset)  {
    auto result = CVector();

    if (m_matrix) {
        result = *m_matrix * offset;
        return result;
    }

    CUtil::TransformPoint(result, m_placement, offset);
    return result;
}

CVector* CEntityGTA::TransformFromObjectSpace(CVector& outPos, const CVector& offset) {
    auto result = TransformFromObjectSpace(offset);
    outPos = result;

    return &outPos;
}

CColModel* CEntityGTA::GetColModel() const {
    if (IsVehicle()) {
        const auto veh = static_cast<const CVehicleGTA*>(this);
        if (veh->m_vehicleSpecialColIndex > -1) {
            return &CVehicleGTA::m_aSpecialColModel[veh->m_vehicleSpecialColIndex];
        }
    }

    return CModelInfo::GetModelInfo(m_nModelIndex)->GetColModel();
}

void CEntityGTA::GetBoundCentre(CVector& outCentre) {
    TransformFromObjectSpace(outCentre, GetColModel()->GetBoundCenter());
}

CVector CEntityGTA::GetBoundCentre() {
    CVector v;
    GetBoundCentre(v);
    return v;
}

bool CEntityGTA::SetupLighting() {
    return false;
}
void CEntityGTA::RemoveLighting(bool bRemove) {

}

void CEntityGTA::CreateRwObject() {
    CHook::CallFunction<void>("_ZN7CEntity14CreateRwObjectEv", this);
}
void CEntityGTA::DeleteRwObject() {
    CHook::CallFunction<void>("_ZN7CEntity14DeleteRwObjectEv", this);
}

bool CEntityGTA::IsVisible() {
    if (!m_pRwObject || !m_bIsVisible)
        return false;

    return GetIsOnScreen();
}
bool CEntityGTA::IsEntityOccluded() {
    return CHook::CallFunction<bool>("_ZN7CEntity16IsEntityOccludedEv", this);
}
bool CEntityGTA::GetIsOnScreen() {
    CVector thisVec;
    GetBoundCentre(thisVec);

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    return TheCamera.IsSphereVisible(&thisVec, CModelInfo::GetModelInfo(m_nModelIndex)->GetColModel()->GetBoundRadius());
}