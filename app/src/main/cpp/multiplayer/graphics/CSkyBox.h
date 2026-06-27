//
// Created by bkuzn on 10.01.2026.
//
#include "main.h"

class CObject;

class CSkyBox {
public:
    static void Process();

    static CObject* m_pSkyBox;
private:

    static uintptr_t RwFrameForAllObjectsCallback(uintptr_t object, CObject *pObject);

    static RpAtomic *ObjectMaterialCallBack(RpAtomic *rpAtomic, CObject *pObject);
};