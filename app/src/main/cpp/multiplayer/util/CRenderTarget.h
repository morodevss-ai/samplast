#pragma once

#include <vector>
#include <stdint.h>
#include "color.h"

class CRenderTarget
{
    static struct RwTexture* m_pResultTexture;
    static struct RwCamera* m_pCamera;
    static struct RpLight* m_pLight;
    static struct RwFrame* m_pFrame;
    static struct RwRaster* m_zBuffer;
    static bool m_bReady;
    static void PreProcessCamera(RwInt32 sizeX, RwInt32 sizeY);
    static void ProcessCamera(RwRGBA* bgcolor, bool b2D);
    static void PostProcessCamera();
    static bool m_bSucessfull;

public:
    static void Begin(RwInt32 sizeX, RwInt32 sizeY, uint32_t bgColor, bool b2D);
    static struct RwTexture* End();
    void shutDown();
    static bool InitialiseScene();
    static void Initialise();
    static bool IsInitialized() { return m_bReady && m_pCamera != nullptr && m_pLight != nullptr && m_pFrame != nullptr; }
};