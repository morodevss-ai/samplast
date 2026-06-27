/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/
#pragma once

#include "Enums/AnimationEnums.h"
#define VALIDATE_SIZE(struc, size) static_assert(sizeof(struc) == size, "Invalid structure size of " #struc)


class CRideAnimData {
public:
    AssocGroupId m_nAnimGroup{};
    float m_fBarSteerAngle;
    float m_fLeanAngle;
    float m_fDesiredLeanAngle;
    float m_fLeanFwd;
    float m_fAnimLeanLeft;
    float m_fAnimLeanFwd;

public:
    CRideAnimData() = default; // 0x6D0B10
    CRideAnimData(AssocGroupId animGroup) : m_nAnimGroup(animGroup) {} // NOTSA
};

VALIDATE_SIZE(CRideAnimData, 0x1C);
