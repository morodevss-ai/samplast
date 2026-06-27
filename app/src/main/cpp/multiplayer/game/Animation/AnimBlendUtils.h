#pragma once

#include "AnimBlendAssociation.h"
#include "game/Plugins/RpAnimBlendPlugin/RpAnimBlend.h"

inline bool BlendOutClumpAnimationByName(RpClump* clump, const char* animName, float blendDelta) {
    if (!clump || !animName || !*animName) {
        return false;
    }

    const auto clumpAddr = reinterpret_cast<uintptr_t>(clump);
    if (clumpAddr < 0x1000 || (clumpAddr & (alignof(void*) - 1)) != 0) {
        return false;
    }

    if (blendDelta > 0.0f) {
        blendDelta = -blendDelta;
    }

    auto* anim = RpAnimBlendClumpGetAssociation(clump, animName);
    if (!anim || anim->IsIndestructible()) {
        return false;
    }

    anim->SetFlag(ANIMATION_FREEZE_LAST_FRAME, true);
    if (anim->m_fBlendDelta > blendDelta) {
        anim->m_fBlendDelta = blendDelta;
    }

    return true;
}

inline void BlendOutClumpAnimations(RpClump* clump, float blendDelta) {
    if (!clump) {
        return;
    }

    const auto clumpAddr = reinterpret_cast<uintptr_t>(clump);
    if (clumpAddr < 0x1000 || (clumpAddr & (alignof(void*) - 1)) != 0) {
        return;
    }

    if (blendDelta > 0.0f) {
        blendDelta = -blendDelta;
    }

    auto blendOut = [blendDelta](CAnimBlendAssociation* anim) {
        anim->SetFlag(ANIMATION_FREEZE_LAST_FRAME, true);
        if (anim->m_fBlendDelta > blendDelta) {
            anim->m_fBlendDelta = blendDelta;
        }
    };

    bool didBlend = false;
    for (auto anim = RpAnimBlendClumpGetFirstAssociation(clump, 0); anim; ) {
        auto* next = RpAnimBlendGetNextAssociation(anim);
        if (!anim->IsIndestructible() && !anim->IsMoving() &&
            !(anim->m_nFlags & ANIMATION_ADD_TO_BLEND)) {
            blendOut(anim);
            didBlend = true;
        }
        anim = next;
    }

    if (didBlend) {
        return;
    }

    for (auto anim = RpAnimBlendClumpGetFirstAssociation(clump, 0); anim; ) {
        auto* next = RpAnimBlendGetNextAssociation(anim);
        if (!anim->IsIndestructible() && !anim->IsMoving()) {
            blendOut(anim);
        }
        anim = next;
    }
}
