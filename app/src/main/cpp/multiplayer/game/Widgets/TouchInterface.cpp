//
// Created by x1y2z on 01.05.2023.
//

#include "../common.h"
#include "TouchInterface.h"
#include "game/RW/RenderWare.h"
#include "game/Enums/OSDeviceForm.h"
#include "../../vendor/armhook/patch.h"

CWidgetGta** CTouchInterface::m_pWidgets;

void CTouchInterface::LoadDefaultConfigFile()
{
    int v0; // r5
    int v1; // r6

    v0 = 2;
    v1 = 0;
    if ( (RsGlobal->maximumWidth / RsGlobal->maximumHeight) >= 1.5 )
    {
        v0 = 3;
        v1 = 1;
    }
    const char* v5[] = {"TouchDefaultTablet4x3.cfg",
                        "TouchDefaultTabletWidescreen.cfg",
                        "TouchDefaultPhone3x2.cfg",
                        "TouchDefaultPhoneWidescreen.cfg",
                        "360Default1280x720.cfg",
                        "360Default960x720.cfg"
    };

    if ( CHook::CallFunction<OSDeviceForm>(g_libGTASA + (VER_x32 ? 0x0026BAD4 + 1 : 0x32155C)) == OSDF_Phone )
        v1 = v0;

    CTouchInterface::LoadFromFile(v5[v1], true);

    if(m_pWidgets && m_pWidgets[WIDGET_CAM_TOGGLE]) {
        m_pWidgets[WIDGET_CAM_TOGGLE]->SetEnabled(true);
    }
}

void __fastcall CTouchInterface::LoadFromFile(const char *pszFilename, bool bFullLoad) {
    assert("f");
}

void CTouchInterface::InjectHooks() {
    SET_TO(m_pWidgets, *(void**)(g_libGTASA + (VER_x32 ? 0x00679474 : 0x850910)));

    if(m_pWidgets && m_pWidgets[WIDGET_CAM_TOGGLE]) {
        m_pWidgets[WIDGET_CAM_TOGGLE]->SetEnabled(true);
    }
}

bool CTouchInterface::IsTouched(WidgetIDs WidgetID, CVector2D *pVecOut, int nFrameCount) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x2B0D2C + 1 : 0x36FC60), WidgetID, pVecOut, nFrameCount);
}

bool CTouchInterface::IsDoubleTapped(WidgetIDs WidgetID, bool bShowEffects, int nFrameCount) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x2B20D8 + 1 : 0x371120), WidgetID, bShowEffects, nFrameCount);
}