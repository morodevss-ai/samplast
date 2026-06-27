//
// Created for ModelsDebugModule
//

#include "Sprite.h"
#include <cmath>

// Простая реализация для преобразования мировых координат в экранные
bool CSprite::CalcScreenCoors(const CVector& worldPos, CVector* outScreenPos,
                              float* outWidth, float* outHeight,
                              bool bCheckMaxDistance, bool bActor) {
    // Это очень упрощенная версия - в реальном проекте здесь должна быть
    // проекция 3D -> 2D с использованием матриц вида и проекции

    // Для примера просто копируем координаты (это неправильно, но для демонстрации)
    if (outScreenPos) {
        outScreenPos->x = worldPos.x;
        outScreenPos->y = worldPos.y;
        outScreenPos->z = worldPos.z;
    }

    if (outWidth) *outWidth = 0;
    if (outHeight) *outHeight = 0;

    // Всегда возвращаем true для демонстрации
    return true;
}