//
// Created for ModelsDebugModule
//

#ifndef RUSSIA_SPRITE_H
#define RUSSIA_SPRITE_H

#include "Vector.h" // Предполагаю, что у вас есть класс CVector

class CSprite {
public:
    // Статический метод для расчета экранных координат
    static bool CalcScreenCoors(const CVector& worldPos, CVector* outScreenPos,
                                float* outWidth, float* outHeight,
                                bool bCheckMaxDistance, bool bActor);
};

#endif //RUSSIA_SPRITE_H