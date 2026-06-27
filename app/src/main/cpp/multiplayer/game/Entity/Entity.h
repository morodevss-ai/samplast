//
// Created for ModelsDebugModule
//

#ifndef RUSSIA_ENTITY_H
#define RUSSIA_ENTITY_H

//#include "ModelInfo.h"
#include "Matrix.h"
#include "Vector.h"
#include "rwcore.h"

// Forward declarations
class CModelInfo;
class CMatrix;

class CEntity {
public:
    // Конструктор/деструктор
    CEntity();
    virtual ~CEntity();

    // Методы доступа к ModelInfo
    CModelInfo* GetModelInfo();
    const CModelInfo* GetModelInfo() const;

    // Поля (должны соответствовать структуре из игры)
    RwMatrix* m_matrix;        // Матрица трансформации
    RwFrame* m_pRwFrame;       // RW frame
    RwObject* m_pRwAtomic;     // RW atomic object
    int m_nModelIndex;         // Индекс модели
    unsigned char m_nSpecialType; // Специальный тип

    // Методы
    CVector GetPosition() const;

    // Статические поля
    static CModelInfo** ms_modelInfoPtrs;
};

#endif //RUSSIA_ENTITY_H