//
// Created by Bkuzn on 23.02.2026.
//

#include "Entity.h"

// Статическая инициализация
CModelInfo** CEntity::ms_modelInfoPtrs = nullptr;

CEntity::CEntity()
        : m_matrix(nullptr)
        , m_pRwFrame(nullptr)
        , m_pRwAtomic(nullptr)
        , m_nModelIndex(-1)
        , m_nSpecialType(0) {
}

CEntity::~CEntity() {
    // Виртуальный деструктор
}

CModelInfo* CEntity::GetModelInfo() {
    if (m_nModelIndex >= 0 && ms_modelInfoPtrs) {
        return ms_modelInfoPtrs[m_nModelIndex];
    }
    return nullptr;
}

const CModelInfo* CEntity::GetModelInfo() const {
    if (m_nModelIndex >= 0 && ms_modelInfoPtrs) {
        return ms_modelInfoPtrs[m_nModelIndex];
    }
    return nullptr;
}

CVector CEntity::GetPosition() const {
    CVector pos(0, 0, 0);
    if (m_matrix) {
        pos.x = m_matrix->pos.x;
        pos.y = m_matrix->pos.y;
        pos.z = m_matrix->pos.z;
    }
    return pos;
}