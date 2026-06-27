//
// Created by bkuzn
//

#include "CJavaGui.h"
#include "../main.h"
#include <jni.h>
#include "../vendor/armhook/patch.h"
#include "jniutil.h"
#include "game/Timer.h"
#include "game/game.h"
#include "net/netgame.h"

jobject CJavaGui::thiz = nullptr;
jclass  CJavaGui::clazz = nullptr;

#define ACTION_CREATE (-1)
#define ACTION_DESTROY (-2)
#define ACTION_GOTOP (-6)
#define ACTION_HIDE (-4)
#define ACTION_ONCLOSED (-5)
#define ACTION_SHOW (-3)

void CJavaGui::Create(int id) {
   /* JNIEnv *env = CJavaWrapper::GetEnv();

    jstring jsonString = env->NewStringUTF("{\"o\": 2}");
    jclass jsonClass = env->FindClass("org/json/JSONObject");
    jmethodID jsonConstructor = env->GetMethodID(jsonClass, "<init>", "(Ljava/lang/String;)V");
    jobject jsonObject = env->NewObject(jsonClass, jsonConstructor, jsonString);

    jmethodID method = env->GetStaticMethodID(CJavaGui::clazz, "receiveUIpacket", "(ILorg/json/JSONObject;)V");

    env->CallStaticVoidMethod(CJavaGui::clazz, method, id, jsonObject);

    env->DeleteLocalRef(jsonString);
    env->DeleteLocalRef(jsonObject);
    env->DeleteLocalRef(jsonClass);*/
    JNIEnv *env = CJavaWrapper::GetEnv();

    jstring ss = env->NewStringUTF("");

    jmethodID method = env->GetStaticMethodID(CJavaGui::clazz, "receiveUiPacket", "(IILjava/lang/String;)V");
    env->CallStaticVoidMethod(CJavaGui::clazz, method, id, -2, ss);

    env->DeleteLocalRef(ss);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_crmp_online_core_Samp_00024Companion_nativeDisconnect(JNIEnv *env, jobject thiz)
{
    // TODO: implement nativeDisconnect()

    //return (*(__int64 (__fastcall **)(_QWORD, __int64, _QWORD))(**(_QWORD **)pNetGame + 0x18LL))(*(_QWORD *)pNetGame, 0xC8LL, 0LL);
}
extern "C"
JNIEXPORT void JNICALL
Java_com_goldrp_game_ui_NativeGui_00024Companion_nativeSendPacket(JNIEnv *env, jobject thiz,
        jint ui_id, jint action_id,
        jstring data) {
    // TODO: implement nativeSendPacket()

    const char *dataStr = env->GetStringUTFChars(data, nullptr);

    FLog("Sending packet: uiId=%d, actionId=%d, data=%s", ui_id, action_id, dataStr);
    env->ReleaseStringUTFChars(data, dataStr);
}