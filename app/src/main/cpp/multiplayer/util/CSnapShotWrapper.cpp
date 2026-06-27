#include "CSnapShotWrapper.h"
#include "CSnapShots.h"
#include "java/jniutil.h"
#include <condition_variable>

// Определения статических членов класса
std::mutex CSnapShotWrapper::queueMutex;
std::queue<CSnapShotWrapper::QueueItem> CSnapShotWrapper::itemQueue;
std::condition_variable queueCondition;

jclass CSnapShotWrapper::clazz = nullptr;
jmethodID CSnapShotWrapper::constructorID = nullptr;
bool CSnapShotWrapper::isProcessing = false;

#include "../game/snapshothelper.h"
#include "game/Models/ModelInfo.h"
#include "game/sprite2d.h"

extern CJavaWrapper* pJavaWrapper;
extern CSnapShotHelper* pSnapShotHelper;

jbyteArray CSnapShotWrapper::ConvertTextureToByte(RwTexture * texture, bool isTexture) {
    JNIEnv* pEnv = pJavaWrapper->GetEnv();
    uint32_t dest = 512 * 512 * 4;
    jbyte* bytes = new jbyte[dest];
    uintptr_t textureBuffer = (uintptr_t)texture->raster;

    FLog("hee1110999");
    ((void (*)(uintptr_t, int))(g_libGTASA + (VER_x32 ? 0x001D0958 + 1 : 0x266024)))(*(uintptr_t *)(textureBuffer + *(uintptr_t *)(g_libGTASA + (VER_x32 ? 0x6B31FC : 0x890108)) + (VER_x32 ? 0x18 : 0x20)), isTexture);

    FLog("khhhhhhhhhhhhh");
    ((void (*)(int, int, int, int, uintptr_t, uintptr_t, jbyte*))(g_libGTASA+ (VER_x32 ? 0x1B4C20+1 : 0x247E98)))(0, 0, 512, 512, 0x1908, 0x1401, bytes);

    FLog("khhhhhhhhhhhhh");
    for (uint32_t i = 0; i < dest; i += 4) {
        if (bytes[i] == 123 && bytes[i + 1] == -100 && bytes[i + 2] == -58) {
            bytes[i] = 0;
            bytes[i + 1] = 0;
            bytes[i + 2] = 0;
            bytes[i + 3] = 0;
        }
    }

    jbyteArray jByteArray = pEnv->NewByteArray(dest);
    pEnv->SetByteArrayRegion(jByteArray, 0, dest, bytes);
    delete[] bytes;

    return jByteArray;
}

void CSnapShotWrapper::SetToImageView(jbyteArray jByteArray, uint32_t width, uint32_t height, jobject imageview) {
    JNIEnv* env = pJavaWrapper->GetEnv();

    if(imageview && clazz){
        jmethodID method = env->GetStaticMethodID(CSnapShotWrapper::clazz, "onNativeRendered", "(Landroid/widget/ImageView;[B)V");
        if (method) {
            env->CallStaticVoidMethod(CSnapShotWrapper::clazz, method, imageview, jByteArray);
        }
    }
}

void CSnapShotWrapper::Process() {
    std::unique_lock<std::mutex> lock(queueMutex);
    if (!itemQueue.empty()) {
        QueueItem queueItem2 = itemQueue.front();

        bool isTexture = false;
        RwTexture *snapshot = nullptr;
        jbyteArray bytes = nullptr;

        FLog("рендер два");

        auto dwModelArray2 = CModelInfo::ms_modelInfoPtrs;
        if (dwModelArray2 && dwModelArray2[queueItem2.id]) {
            FLog("рендер %i", queueItem2.id);
            switch (queueItem2.type) {
                case SNAP_PED:
                    snapshot = CSnapShots::CreatePedSnapShot(
                            queueItem2.id, 0xFFFFFFFF, &queueItem2.vecOffset, queueItem2.zoom);
                    isTexture = true;
                    break;
                case SNAP_CAR:
                    FLog("рендер кар");
                    snapshot = CSnapShots::CreateVehicleSnapShot(
                            queueItem2.id, 0xFFFFFFFF, &queueItem2.vecOffset, queueItem2.zoom,
                            queueItem2.color1, queueItem2.color2);
                    isTexture = true;
                    break;
                case SNAP_OBJ:
                    snapshot = CSnapShots::CreateObjectSnapShot(
                            queueItem2.id, 0xFFFFFFFF, &queueItem2.vecOffset, queueItem2.zoom);
                    isTexture = true;
                    break;
                case SNAP_TEXTURE:
                    snapshot = CSnapShots::CreateTextureSnapShot("vehiclelights128");
                    isTexture = true;
                    break;
                default:
                    snapshot = nullptr;
                    isTexture = false;
                    break;
            }
        } else {
            if (queueItem2.type == SNAP_TEXTURE) {
                snapshot = CSnapShots::CreateTextureSnapShot("vehiclelights128");
                isTexture = true;
            } else {
                snapshot = CSnapShots::CreateObjectSnapShot(18631, 0xFFFFFFFF, &queueItem2.vecOffset, 0.8f);
                isTexture = true;
            }
        }

        if (snapshot) {
            FLog("yess");
            bytes = CSnapShotWrapper::ConvertTextureToByte(snapshot, isTexture);
            if (bytes == nullptr) {
                FLog("Error converting texture to byte array.");
            } else {
                try {
                    CSnapShotWrapper::SetToImageView(bytes, snapshot->raster->width, snapshot->raster->height, queueItem2.ImageView);
                } catch (const std::exception& e) {
                    FLog("Error setting texture to ImageView: %s", e.what());
                }
            }
            RwTextureDestroy(snapshot);
        } else {
            FLog("No generated tex for snap. type = %d, model = %d", queueItem2.type, queueItem2.id);
        }
        itemQueue.pop();
    }
}

void CSnapShotWrapper::AddToQueue(QueueItem item) {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        itemQueue.push(item);
    }
    queueCondition.notify_one();
}

void CSnapShotWrapper::startProcess(jobject image_view, int type, int modelid,
                                    int color1, int color2, float zoom,
                                    float off_set_x, float off_set_y, float off_set_z) {

    QueueItem item;
    item.type = type;
    item.id = modelid;
    item.ImageView = image_view;
    item.color1 = color1;
    item.color2 = color2;
    item.zoom = zoom;
    item.vecOffset.X = off_set_x;
    item.vecOffset.Y = off_set_y;
    item.vecOffset.Z = off_set_z;

    AddToQueue(item);
    isProcessing = true;
}