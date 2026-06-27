//
// Created by bkuzn
//

#ifndef X64_CJAVAGUI_H
#define X64_CJAVAGUI_H

#include <jni.h>
#include "JavaGuiCommon.h"

class CJavaGui {
public:
    static jobject thiz;

    static void Create(int id);

    static jclass clazz;
};


#endif //X64_CJAVAGUI_H
