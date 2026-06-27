#include "CRenderTarget.h"
#include "../main.h"
#include "game/RW/rpworld.h"
#include "game/Scene.h"
#include "patch.h"
#include "game/util.h"
#include "game/VisibilityPlugins.h"

struct RwTexture* CRenderTarget::m_pResultTexture = nullptr;
bool CRenderTarget::m_bReady = false;
struct RwCamera* CRenderTarget::m_pCamera = nullptr;
struct RpLight* CRenderTarget::m_pLight = nullptr;
struct RwFrame* CRenderTarget::m_pFrame = nullptr;
struct RwRaster* CRenderTarget::m_zBuffer = nullptr;
bool CRenderTarget::m_bSucessfull = false;

bool CRenderTarget::InitialiseScene()
{
    FLog("CRenderTarget InitialiseScene()!");

    CRenderTarget::m_pLight = RpLightCreate(2);
    if (CRenderTarget::m_pLight == nullptr)
    {
        FLog("CRenderTarget Light not created!");
        return false;
    }

    float rwColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    ((struct RpLight* (*)(struct RpLight*, float*))(g_libGTASA + (VER_x32 ? 0x216746 + 1 : 0x2BD930)))(m_pLight, rwColor);

    CRenderTarget::m_pCamera = ((struct RwCamera* (*)())(g_libGTASA + (VER_x32 ? 0x001D5EE0 + 1 : 0x26D454)))();
    if (CRenderTarget::m_pCamera == nullptr)
    {
        FLog("CRenderTarget Camera not created!");
        return false;
    }

    CRenderTarget::m_pFrame = RwFrameCreate();
    if (CRenderTarget::m_pFrame == nullptr)
    {
        FLog("CRenderTarget frame not created!");
        return false;
    }

    float v[3] = { 0.0f, 0.0f, 50.0f };
    ((RwFrame*(*)(struct RwFrame*, float*, int))(g_libGTASA + (VER_x32 ? 0x001D8614 + 1 : 0x270060)))(CRenderTarget::m_pFrame, v, 1);

    v[0] = 1.0f; v[1] = 0.0f; v[2] = 0.0f;
    ((RwFrame*(*)(struct RwFrame*, float*, float, int))(g_libGTASA + (VER_x32 ? 0x001D8728 + 1 : 0x270204)))(CRenderTarget::m_pFrame, v, 90.0f, 1);

    ((void(*)(struct RwCamera*, struct RwFrame*))(g_libGTASA + (VER_x32 ? 0x001DCF64 + 1 : 0x275CF0)))(CRenderTarget::m_pCamera, CRenderTarget::m_pFrame);

    ((struct RwCamera*(*)(struct RwCamera*, float))(g_libGTASA +(VER_x32 ?  0x001D5ACC + 1 : 0x26D034)))(CRenderTarget::m_pCamera, 300.0f);

    ((struct RwCamera* (*)(struct RwCamera*, float))(g_libGTASA + (VER_x32 ? 0x001D5A38 + 1 : 0x26CF8C)))(CRenderTarget::m_pCamera, 0.01f);

    float view[2] = { 0.5f, 0.5f };
    ((struct RwCamera* (*)(struct RwCamera*, float*))(g_libGTASA + (VER_x32 ? 0x001D5E04 + 1 : 0x26D330)))(CRenderTarget::m_pCamera, view);

    ((struct RwCamera* (*)(struct RwCamera*, int))(g_libGTASA + (VER_x32 ? 0x001D5D28 + 1 : 0x26D24C)))(CRenderTarget::m_pCamera, 1);

    if (Scene.m_pRpWorld)
    {
        CHook::CallFunction<uintptr_t>(g_libGTASA + (VER_x32 ? 0x0021DF84 + 1 : 0x2C78F0), Scene.m_pRpWorld, (struct RwCamera*)CRenderTarget::m_pCamera);
    }
    else
    {
        FLog("CRenderTarget Scene.m_pRpWorld is null!");
        return false;
    }

    return true;
}

void CRenderTarget::Initialise()
{
    CRenderTarget::m_pResultTexture = nullptr;
    CRenderTarget::m_pCamera = nullptr;
    CRenderTarget::m_pLight = nullptr;
    CRenderTarget::m_pFrame = nullptr;
    CRenderTarget::m_zBuffer = nullptr;
    m_bReady = false;

    if(InitialiseScene())
    {
        m_bReady = true;
        FLog("CRenderTarget initialized successfully");
    }
    else
    {
        FLog("CRenderTarget initialization failed");
    }
}

void CRenderTarget::Begin(RwInt32 sizeX, RwInt32 sizeY, uint32_t bgColor, bool b2D)
{
    if (!m_bReady || !m_pCamera || !m_pLight || !m_pFrame)
    {
        FLog("CRenderTarget::Begin: Render target not ready (m_bReady=%d, camera=%p, light=%p, frame=%p)",
             m_bReady, m_pCamera, m_pLight, m_pFrame);
        m_bSucessfull = false;
        return;
    }

    if(m_zBuffer)
    {
        RwRasterDestroy(m_zBuffer);
        m_zBuffer = nullptr;
    }

    m_zBuffer = RwRasterCreate(sizeX, sizeY, 0, rwRASTERTYPEZBUFFER);
    if (m_zBuffer == nullptr)
    {
        FLog("CRenderTarget::Begin: Failed to create zBuffer");
        m_bSucessfull = false;
        return;
    }

    if (m_pCamera)
    {
        *(RwRaster**)((uintptr_t)m_pCamera + 0x64) = m_zBuffer;
    }

    struct RwRaster* pRaster = RwRasterCreate(sizeX, sizeY, 32, rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE);
    if(!pRaster)
    {
        FLog("CRenderTarget::Begin: Failed to create raster");
        RwRasterDestroy(m_zBuffer);
        m_zBuffer = nullptr;
        m_bSucessfull = false;
        return;
    }

    if (m_pCamera)
    {
        *(RwRaster**)((uintptr_t)m_pCamera + (VER_x32 ? 0x60 : 0x80)) = pRaster;
        CVisibilityPlugins::SetRenderWareCamera(m_pCamera);
    }

    RwCameraClear(m_pCamera, reinterpret_cast<RwRGBA *>(&bgColor), 3);

    if (!RwCameraBeginUpdate(m_pCamera))
    {
        FLog("CRenderTarget::Begin: RwCameraBeginUpdate failed");
        RwRasterDestroy(m_zBuffer);
        m_zBuffer = nullptr;
        RwRasterDestroy(pRaster);
        m_bSucessfull = false;
        return;
    }

    m_bSucessfull = true;

    if (Scene.m_pRpWorld && m_pLight)
    {
        RpWorldAddLight(Scene.m_pRpWorld, m_pLight);
    }

    if (b2D)
    {
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
        RwRenderStateSet(rwRENDERSTATEBORDERCOLOR, (void*)0);
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTION, (void*)rwALPHATESTFUNCTIONGREATER);
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)2);
        RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEARMIPLINEAR);
        RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);

        DefinedState2d();
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)true);
        RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODENASHADEMODE);
        RwRenderStateSet(rwRENDERSTATEALPHATESTFUNCTIONREF, (void*)0);
        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODENACULLMODE);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)false);

        ((void(*) (void))(g_libGTASA + (VER_x32 ? 0x005D0C10 + 1 : 0x6F4EC4)))();
    }
}

RwTexture* CRenderTarget::End()
{
    if (!m_bReady || !m_pCamera)
    {
        FLog("CRenderTarget::End: Render target not ready");
        return nullptr;
    }

    if (m_bSucessfull)
    {
        RwCameraEndUpdate(m_pCamera);

        if (Scene.m_pRpWorld && m_pLight)
        {
            RpWorldRemoveLight(Scene.m_pRpWorld, m_pLight);
        }
    }

    if(!m_pCamera->frameBuffer)
    {
        FLog("CRenderTarget::End: frameBuffer is null");
        return nullptr;
    }

    struct RwTexture* pTexture = RwTextureCreate(m_pCamera->frameBuffer);
    if(!pTexture)
    {
        FLog("CRenderTarget::End: Failed to create texture");
        return nullptr;
    }

    ((RwTexture*(*)(struct RwTexture*, const char*))(g_libGTASA + (VER_x32 ? 0x001DB820 + 1 : 0x274000)))(pTexture, "rtarget");

    if (m_zBuffer)
    {
        RwRasterDestroy(m_zBuffer);
        m_zBuffer = nullptr;
    }

    if (m_pCamera->frameBuffer)
    {
        RwRasterDestroy(m_pCamera->frameBuffer);
        m_pCamera->frameBuffer = nullptr;
    }

    return pTexture;
}

void CRenderTarget::shutDown()
{
    if (m_pLight)
    {
        RpLightDestroy(m_pLight);
        m_pLight = nullptr;
    }

    if (m_pCamera && m_pFrame)
    {
        ((void(*)(struct RwCamera*, struct RwFrame*))(g_libGTASA + (VER_x32 ? 0x001DCF64 + 1 : 0x275CF0)))(m_pCamera, nullptr);
    }

    if (m_pFrame)
    {
        ((RwBool(*)(struct RwFrame*))(g_libGTASA + 0x001D83EC + 1))(m_pFrame);
        m_pFrame = nullptr;
    }

    if (m_zBuffer)
    {
        RwRasterDestroy(m_zBuffer);
        m_zBuffer = nullptr;
    }

    if (m_pCamera)
    {
        ((RwBool(*)(struct RwCamera*))(g_libGTASA + 0x001D5EA0 + 1))(m_pCamera);
        m_pCamera = nullptr;
    }

    m_bReady = false;
}