#pragma once

#include "Entity/CEntityGTA.h"
#include "Entity/CVehicleGTA.h"

#include "game/common.h"
#include "Models/BaseModelInfo.h"

enum eRendererVisibility {
    RENDERER_INVISIBLE = 0,
    RENDERER_VISIBLE,
    RENDERER_CULLED,
    RENDERER_STREAMME
};

constexpr auto MAX_INVISIBLE_ENTITY_PTRS = 1500;
constexpr auto MAX_VISIBLE_ENTITY_PTRS   = 10000;
constexpr auto MAX_VISIBLE_LOD_PTRS      = 10000;
constexpr auto MAX_VISIBLE_SUPERLOD_PTRS = 500;

class CRenderer {
public:
    static inline bool ms_bRenderOutsideTunnels;
    static inline bool m_loadingPriority;
    static inline CVector ms_vecCameraPosition{};

    static inline CEntityGTA* ms_aVisibleEntityPtrs[MAX_VISIBLE_ENTITY_PTRS];
    static inline int32 ms_nNoOfVisibleEntities;

    static inline CEntityGTA* ms_aVisibleLodPtrs[MAX_VISIBLE_LOD_PTRS];
    static inline int32 ms_nNoOfVisibleLods{};

    static inline float ms_lodDistScale;

    static inline float ms_lowLodDistScale;
    static inline float ms_fFarClipPlane;
    static inline bool ms_bRenderTunnels;

public:
    static void InjectHooks();

    static void PreRender();
    static void ConstructRenderList();
    static void RenderFadingInEntities();
    static void RenderEverythingBarRoads();
    static void RenderOneNonRoad(CEntityGTA* entity);

    static void RenderFadingInUnderwaterEntities();
    static void RenderRoads();

    static auto GetVisibleLodPtrs()      { return std::span{ ms_aVisibleLodPtrs,      (size_t)ms_nNoOfVisibleLods }; }
    static auto GetVisibleEntityPtrs()   { return std::span{ ms_aVisibleEntityPtrs,   (size_t)ms_nNoOfVisibleEntities }; }

    static void AddToLodRenderList(CEntityGTA* entity, float distance);
    static void AddEntityToRenderList(CEntityGTA* entity, float fDistance);

    static void UpdateLodDistScale(int value);
    static void ScanWorld();

    static int32 SetupMapEntityVisibility(CEntityGTA* entity, CBaseModelInfo* baseModelInfo, float fDistance, bool bIsTimeInRange);

public:
    static inline float mobileDrawDistance;
};
