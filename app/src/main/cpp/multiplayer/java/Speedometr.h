//
// Created by bkuzn on 04.05.2026.
//

#ifndef GRX64_CSPEEDOMETR_H
#define GRX64_CSPEEDOMETR_H

#include <jni.h>

static class CSpeedometr {
public:

    enum
    {
        BUTTON_ENGINE,
        BUTTON_LIGHT,
        BUTTON_DOOR
    };

    static bool bIsShow;

    static void hide();

    static void show();

    static jclass clazz;

    static void update();

    static float fFuel;
    static int iMilliage;

    static void tempToggle(bool toggle);

    static void updateSpeed();

    static jobject thiz;
};

#endif //GRX64_CSPEEDOMETR_H