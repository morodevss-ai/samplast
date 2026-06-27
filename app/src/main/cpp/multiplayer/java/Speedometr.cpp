//
// Created by bkuzn on 04.05.2026.
//
#include "Speedometr.h"
#include "main.h"

#include "game/game.h"
#include "net/netgame.h"
#include "java/jniutil.h"

extern CNetGame *pNetGame;

jclass  CSpeedometr::clazz = nullptr;
jobject CSpeedometr::thiz = nullptr;
bool    CSpeedometr::bIsShow = false;
float   CSpeedometr::fFuel = 0.0f;
int     CSpeedometr::iMilliage = 0;

extern "C"
JNIEXPORT void JNICALL
Java_com_goldrp_game_ui_hud_Speedometr_nativeInit(JNIEnv *env, jobject thiz) {
    // TODO: implement nativeInit()

    CSpeedometr::thiz = env->NewGlobalRef(thiz);
}

void CSpeedometr::show() {
    if (CSpeedometr::bIsShow) return;

    JNIEnv *env = CJavaWrapper::GetEnv();

    jmethodID method = env->GetMethodID(CSpeedometr::clazz, "show", "()V");
    env->CallVoidMethod(CSpeedometr::thiz, method);
    CSpeedometr::bIsShow = true;
}

void CSpeedometr::tempToggle(bool toggle) {
    if (CSpeedometr::thiz == nullptr) return;
    CSpeedometr::bIsShow = toggle;

    JNIEnv *env = CJavaWrapper::GetEnv();

    jmethodID method = env->GetMethodID(clazz, "tempToggle", "(Z)V");
    env->CallVoidMethod(CSpeedometr::thiz, method, toggle);
}

void CSpeedometr::hide() {
    if (CSpeedometr::thiz == nullptr) return;
    JNIEnv *env = CJavaWrapper::GetEnv();

    jmethodID method = env->GetMethodID(clazz, "hide", "()V");
    env->CallVoidMethod(CSpeedometr::thiz, method);

    CSpeedometr::bIsShow = false;
}

void CSpeedometr::update() {
    if (CSpeedometr::thiz == nullptr) return;
    JNIEnv *env = CJavaWrapper::GetEnv();

    CLocalPlayer *pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
    if (!pPlayer)return;

    CPlayerPed *pPed = pPlayer->GetPlayerPed();
    if (!pPed) return;

    if (!pPed->IsInVehicle()) return;

    CVehicle *pVehicle = pPed->GetCurrentVehicle();

    jmethodID method = env->GetMethodID(clazz, "update", "(IIIII)V");
    env->CallVoidMethod(
            CSpeedometr::thiz,
            method,
            (int) CSpeedometr::fFuel,
            (int) pVehicle->GetHealth(),
            pVehicle->m_iEngineState,
            (int) pVehicle->m_iLightState,
            (int) pVehicle->m_bDoorsLocked
    );
    CSpeedometr::updateSpeed();
}

void CSpeedometr::updateSpeed() {
    if (CSpeedometr::thiz == nullptr) return;
    JNIEnv *env = CJavaWrapper::GetEnv();

    CLocalPlayer *pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
    if (!pPlayer)return;

    CPlayerPed *pPed = pPlayer->GetPlayerPed();
    if (!pPed) return;

    if (!pPed->IsInVehicle()) return;

    CVehicle *pVehicle = pPed->GetCurrentVehicle();

    jmethodID method = env->GetMethodID(clazz, "updateSpeed", "(I)V");
    auto vecSpeed = pVehicle->m_pVehicle->GetMoveSpeed();
    auto speed = std::sqrt(vecSpeed.x * vecSpeed.x + vecSpeed.y * vecSpeed.y + vecSpeed.z * vecSpeed.z) * 179.1f;

    env->CallVoidMethod(
            CSpeedometr::thiz,
            method,
            (int) speed
    );
}

extern "C"
JNIEXPORT void JNICALL
Java_com_goldrp_game_ui_hud_Speedometr_nativeSendClick(JNIEnv *env, jobject thiz,
                                                                  jint id) {
    // TODO: implement nativeSendClick()

    switch (id) {
        case CSpeedometr::BUTTON_ENGINE: {
            pNetGame->SendChatCommand("/en");
            break;
        }
    }
}