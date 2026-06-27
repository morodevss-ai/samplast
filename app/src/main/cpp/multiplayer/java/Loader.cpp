//
// Created by bkuzn
//

#include <jni.h>
#include "Loader.h"
#include "java/CJavaGui.h"
#include "java//Speedometr.h"
#include "main.h"

extern void Log(const char* fmt, ...);

jobject CLoader::thiz = nullptr;
jclass  CLoader::clazz = nullptr;

void CLoader::initJavaClasses(JavaVM* pjvm)
{
    JNIEnv* env = nullptr;
    pjvm->GetEnv((void**)& env, JNI_VERSION_1_6);

    //CDialog::clazz = env->FindClass("com/crmp/online/gui/dialogs/Dialog");
    //CDialog::clazz = (jclass) env->NewGlobalRef( CDialog::clazz );

    CJavaGui::clazz = env->FindClass("com/goldrp/game/NativeGuiManager");
    CJavaGui::clazz = (jclass) env->NewGlobalRef(CJavaGui::clazz );

    CSpeedometr::clazz = env->FindClass("com/goldrp/game/ui/hud/Speedometr");
    if (CSpeedometr::clazz == nullptr) {
        FLog("Failed to create global ref for Speedometr class");
        return;
    }

    CSpeedometr::clazz = (jclass) env->NewGlobalRef( CSpeedometr::clazz );

    //CLoader::clazz = env->FindClass("com/crmp/online/gui/main_menu/utils/SettingMenuUtil");
    CLoader::clazz = (jclass) env->NewGlobalRef( CLoader::clazz );
}