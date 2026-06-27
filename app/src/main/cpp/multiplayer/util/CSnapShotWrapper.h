//
// Created by Error on 18.10.2024.
//

#ifndef BRSELL_CSNAPSHOTWRAPPER_H
#define BRSELL_CSNAPSHOTWRAPPER_H
#include "../main.h"
#include "../game/game.h"
#include "../game/RW/RenderWare.h"
#include "game/Core/Vector.h"
#include "game/Core/Vector2D.h"
#include <mutex>
#include <queue>

struct VECTOR {
    float X, Y, Z;
};

class CSnapShotWrapper {
public:
    enum SnapType{
        SNAP_PED,
        SNAP_CAR,
        SNAP_OBJ,
        SNAP_PLATE,
        SNAP_TEXTURE,
        SNAP_PLAYER
    };

    struct QueueItem{
        uint8_t type;
        int id;
        jobject ImageView;
        VECTOR vecOffset;
        float zoom;
        int color1;
        int color2;

        // Конструктор для инициализации по умолчанию
        QueueItem() : type(0), id(0), ImageView(nullptr), zoom(0), color1(0), color2(0) {
            vecOffset.X = vecOffset.Y = vecOffset.Z = 0;
        }
    };

    struct PlateItem{
        uint32_t dwType;
        char szNumber[30];
        char szRegion[10];
    };

    static jmethodID constructorID;
    static jclass clazz;
    static std::mutex queueMutex;
    static bool isProcessing;
    static std::queue<QueueItem> itemQueue;

    static jbyteArray ConvertTextureToByte(RwTexture * texture, bool isTexture);
    static void SetToImageView(jbyteArray jByteArray, uint32_t width, uint32_t height, jobject imageview);
    static void Process();
    static void AddToQueue(QueueItem item);
    static void startProcess(jobject image_view,
                             int type, int modelid,
                             int color1, int color2,
                             float zoom,
                             float off_set_x,
                             float off_set_y,
                             float off_set_z);
};

#endif //BRSELL_CSNAPSHOTWRAPPER_H