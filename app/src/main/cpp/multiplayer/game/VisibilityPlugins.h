#pragma once

#include "Entity/CEntityGTA.h"
#include "Entity/CVehicleGTA.h"

#include "common.h"
#include "LinkList.h"
#include "Models/BaseModelInfo.h"

class CPed;

enum eAtomicComponentFlag {
    ATOMIC_IS_NOT_PRESENT = 0x0,
    ATOMIC_IS_OK_STATE = 0x1,
    ATOMIC_IS_DAM_STATE = 0x2,
    ATOMIC_IS_LEFT = 0x4,
    ATOMIC_IS_RIGHT = 0x8,
    ATOMIC_IS_FRONT = 0x10,
    ATOMIC_IS_REAR = 0x20,
    ATOMIC_HAS_ALPHA = 0x40,
    ATOMIC_CULL = 0x80,
    ATOMIC_IS_REAR_DOOR = 0x100,
    ATOMIC_IS_FRONT_DOOR = 0x200,
    ATOMIC_RENDER_ALWAYS = 0x400, // enabled for opened components (doors)
    ATOMIC_IS_REPLACEMENT_UPGRADE = 0x800,
    ATOMIC_IS_DOOR_WINDOW_OPENED = 0x1000,
    ATOMIC_DISABLE_REFLECTIONS = 0x2000,
    // ATOMIC_HAS_DYNAMIC_TEXTURES  = 0x2000, // TODO: WTF is this?
    ATOMIC_IS_BLOWN_UP = 0x4000,
    ATOMIC_VEHCOMP_15 = 0x8000
};

class CVisibilityPlugins {
    static const int32 TOTAL_ALPHA_LISTS = 20; // SA 20
    static const int32 TOTAL_ALPHA_BOAT_ATOMIC_LISTS = 40; // SA 20
    static const int32 TOTAL_ALPHA_ENTITY_LISTS = 400; // SA 200
    static const int32 TOTAL_ALPHA_UNDERWATER_ENTITY_LISTS = 300; // SA 100
    static const int32 TOTAL_ALPHA_DRAW_LAST_LISTS = 100; // SA 50
    static const int32 TOTAL_WEAPON_PEDS_FOR_PC = 100; // SA 100

    struct AlphaObjectInfo {
        union {
            RpAtomic* m_atomic;
            CEntityGTA* m_entity;
        };
        void* m_pCallback;
        float m_distance; // alpha
    };

public:
    static inline float     ms_pedLodDist{};
    static inline RwV3d*    ms_pCameraPosn{};

    static inline float     ms_cullCompsDist;
    static inline float     ms_cullBigCompsDist;
    static inline float     ms_vehicleLod0RenderMultiPassDist;
    static inline float     ms_vehicleLod0Dist;
    static inline float     ms_vehicleLod1Dist;
    static inline float     ms_bigVehicleLod0Dist;
    static inline float     gVehicleDistanceFromCamera;

    static inline CLinkList<AlphaObjectInfo> m_alphaList;
    static inline CLinkList<AlphaObjectInfo> m_alphaEntityList;
    static inline CLinkList<AlphaObjectInfo> m_alphaBoatAtomicList;
    static inline CLinkList<AlphaObjectInfo> m_alphaUnderwaterEntityList;
    static inline CLinkList<AlphaObjectInfo> m_alphaReallyDrawLastList;
    static inline CLinkList<CPed*> ms_weaponPedsForPC;

public:
    static void InjectHooks();

    static void Initialise();
    static void SetRenderWareCamera(RwCamera* camera);
    static float GetDistanceSquaredFromCamera(RwFrame* frame);
    static float GetDotProductWithCameraVector(RwMatrix* atomicMatrix, RwMatrix* clumpMatrix, uint16 flags);
    static inline float GetDistanceSquaredFromCamera1(RwFrame* frame) { return GetDistanceSquaredFromCamera(frame); };
    static void RenderReallyDrawLastObjects();
    static RpAtomic* DefaultAtomicRenderCallback(RpAtomic* atomic);
    static void SetAtomicRenderCallback(RpAtomic* atomic, RpAtomicCallBackRender renderCB);

    static bool InsertEntityIntoSortedList(CEntityGTA* entity, float distance);
    static bool InsertEntityIntoUnderwaterEntities(CEntityGTA* entity, float distance);

    static RpAtomic* RenderVehicleHiDetailCB(RpAtomic* atomic);
    static RpAtomic* RenderVehicleReallyLowDetailCB(RpAtomic* atomic);
    static RpAtomic* RenderVehicleHiDetailAlphaCB(RpAtomic* atomic);
    static RpAtomic* RenderHeliRotorAlphaCB(RpAtomic* atomic);
    static RpAtomic* RenderHeliTailRotorAlphaCB(RpAtomic* atomic);
    static RpAtomic* RenderVehicleReallyLowDetailCB_BigVehicle(RpAtomic* atomic);
    static RpAtomic* RenderVehicleHiDetailAlphaCB_BigVehicle(RpAtomic* atomic);
    static RpAtomic* RenderVehicleHiDetailCB_BigVehicle(RpAtomic* atomic);
    static RpAtomic* RenderVehicleHiDetailCB_Boat(RpAtomic* atomic);
    static RpAtomic* RenderVehicleLoDetailCB_Boat(RpAtomic* atomic);
    static RpAtomic* RenderVehicleHiDetailAlphaCB_Boat(RpAtomic* atomic);
    static RpAtomic* RenderTrainHiDetailAlphaCB(RpAtomic* atomic);
    static RpAtomic* RenderTrainHiDetailCB(RpAtomic* atomic);

    static RpAtomic* RenderWeaponCB(RpAtomic* atomic);
    static void RenderWeaponPedsForPC();

    static float GetDistanceSquaredFromCamera(const CVector* pos);

    static RpAtomic *RenderPedCB(RpAtomic *atomic);

    static void SetClumpAlpha(RpClump* clump, int32 alpha);
    static int32 GetClumpAlpha(RpClump *clump);
    static uint16 GetAtomicId(RpAtomic* atomic);

    static void SetAtomicFlag(RpAtomic* atomic, uint16 flag);
    static void ClearAtomicFlag(RpAtomic* atomic, uint16 flag);

    static void RenderAlphaAtomic(RpAtomic *atomic, int32 alpha);
    static void SetupVehicleVariables(RpClump* clump);
    static void RenderFadingEntities();

    static void RenderEntity(CEntityGTA* entity, float distance);
    static int32 CalculateFadingAtomicAlpha(CBaseModelInfo* modelInfo, CEntityGTA* entity, float distance);

    static void RenderFadingAtomic(CBaseModelInfo* modelInfo, RpAtomic* atomic, int32 alpha);
    static void RenderFadingClump(CBaseModelInfo* modelInfo, RpClump* clump, int32 alpha);
    static void InitAlphaAtomicList();
    static void RenderAlphaAtomics();
    static RpAtomic* RenderAtomicWithAlphaCB(RpAtomic* atomic, void* data);

};

