#include "Hud.h"
#include <jni.h>
#include <thread>
#include <chrono>
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "java/jniutil.h"
#include "settings.h"
#include "Speedometr.h"

extern CNetGame* pNetGame;
extern CJavaWrapper *g_pJavaWrapper;
extern CGame *pGame;

bool CHud::bIsShow = false;
bool CHud::bIsShowEnterExitButt = false;
jobject CHud::thiz = nullptr;
int CHud::iWantedLevel = 0;
int CHud::iLocalMoney = 0;
CVector2D CHud::radarBgPos1;
CVector2D CHud::radarBgPos2;
CVector2D CHud::radarPos;
float CHud::radarSize = 0.0f;

static jmethodID jUpdateHudInfo = nullptr;
static jmethodID jUpdateAmmo = nullptr;
static std::thread* ammoThread = nullptr;
static bool ammoThreadRunning = false;

static void AmmoUpdateLoop() {
    while (ammoThreadRunning) {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        if (CHud::bIsShow) {
            JNIEnv* env = g_pJavaWrapper->GetEnv();
            if (env && jUpdateAmmo) {
                CHud::UpdateAmmo();
            }
        }
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_goldrp_game_ui_hud_Hud_HudInit(JNIEnv *env, jobject thiz) {
CHud::thiz = env->NewGlobalRef(thiz);
jUpdateHudInfo = env->GetMethodID(env->GetObjectClass(thiz), "UpdateHudInfo", "(II)V");
jUpdateAmmo = env->GetMethodID(env->GetObjectClass(thiz), "UpdateAmmo", "(III)V");

if (!ammoThreadRunning) {
ammoThreadRunning = true;
ammoThread = new std::thread(AmmoUpdateLoop);
}
}

void CHud::toggleAll(bool toggle, bool isChat) {
    if(toggle == bIsShow) return;
    bIsShow = toggle;

    JNIEnv *env = CJavaWrapper::GetEnv();
    if(!env) return;

    jclass clazz = env->GetObjectClass(thiz);
    jmethodID method = env->GetMethodID(clazz, "toggleAll", "(ZZ)V");
    if(method) env->CallVoidMethod(thiz, method, toggle, isChat);
}

void CHud::UpdateHudInfo() {
    if(!bIsShow) return;

    static int tick = 0;
    if(++tick < 20) return;
    tick = 0;

    CPlayerPed* pPed = pGame->FindPlayerPed();
    if(!pPed || !pPed->m_pPed) return;

    JNIEnv* env = CJavaWrapper::GetEnv();
    if(!env || !jUpdateHudInfo) return;

    env->CallVoidMethod(thiz, jUpdateHudInfo, (int)pPed->GetHealth(), (int)pPed->GetArmour());
}

void CHud::UpdateAmmo() {
    if(!bIsShow) return;

    CPlayerPed* pPed = pGame->FindPlayerPed();
    if(!pPed || !pPed->m_pPed) return;

    int activeSlot = pPed->m_pPed->m_nActiveWeaponSlot;
    if(activeSlot < 0 || activeSlot >= 13) return;

    CWeapon* pWeapon = &pPed->m_pPed->m_aWeapons[activeSlot];
    if(!pWeapon) return;

    JNIEnv* env = g_pJavaWrapper->GetEnv();
    if(!env || !jUpdateAmmo) return;

    env->CallVoidMethod(thiz, jUpdateAmmo, (int)pWeapon->dwType, (int)pWeapon->dwAmmo, (int)pWeapon->dwAmmoInClip);
}

void CHud::ForceUpdateAmmo() {
    UpdateAmmo();
}

void CHud::UpdateWanted() {
    JNIEnv* env = CJavaWrapper::GetEnv();
    if(!env) return;

    jclass clazz = env->GetObjectClass(thiz);
    jmethodID method = env->GetMethodID(clazz, "UpdateWanted", "(I)V");
    if(method) env->CallVoidMethod(thiz, method, iWantedLevel);
}

void CHud::UpdateMoney() {
    JNIEnv* env = CJavaWrapper::GetEnv();
    if(!env) return;

    jclass clazz = env->GetObjectClass(thiz);
    jmethodID method = env->GetMethodID(clazz, "updateMoney", "(I)V");
    if(method) env->CallVoidMethod(thiz, method, iLocalMoney);
}

void CHud::toggleLogo(bool toggle) {
    JNIEnv* env = CJavaWrapper::GetEnv();
    if(!env) return;

    jclass clazz = env->GetObjectClass(thiz);
    jmethodID method = env->GetMethodID(clazz, "toggleLogo", "(Z)V");
    if(method) env->CallVoidMethod(thiz, method, toggle);
}
/*
bool CHud::NeededRenderPassengerButton() {
    CPedGTA* pPed = CGame::FindPlayerPed();
    if(bIsShowEnterExitButt && !pPed->m_pPed->IsInVehicle())
        return true;

    return false;
}
*/
void CHud::updatePlayerInfo(int id, char *name) {
    if(!name) return;

    JNIEnv* env = g_pJavaWrapper->GetEnv();
    if(!env) return;

    jclass clazz = env->GetObjectClass(thiz);
    jmethodID method = env->GetMethodID(clazz, "updatePlayerInfo", "(Ljava/lang/String;I)V");
    if(!method) return;

    jstring jname = env->NewStringUTF(name);
    env->CallVoidMethod(thiz, method, jname, id);
    env->DeleteLocalRef(jname);
}

extern "C"
JNIEXPORT void JNICALL Java_com_goldrp_game_ui_hud_Hud_alt(JNIEnv *env, jobject thiz) {
LocalPlayerKeys.bKeys[ePadKeys::KEY_WALK] = true;
}

extern "C"
JNIEXPORT void JNICALL Java_com_goldrp_game_ui_hud_Hud_N(JNIEnv *env, jobject thiz) {
LocalPlayerKeys.bKeys[ePadKeys::KEY_NO] = true;
}

extern "C"
JNIEXPORT void JNICALL Java_com_goldrp_game_ui_hud_Hud_SetRadarBgPos(JNIEnv *env, jobject thiz,
        jfloat x1, jfloat y1, jfloat x2, jfloat y2) {
CHud::radarBgPos1.x = x1;
CHud::radarBgPos1.y = y1;
CHud::radarBgPos2.x = x2;
CHud::radarBgPos2.y = y2;
}

extern "C"
JNIEXPORT void JNICALL Java_com_goldrp_game_ui_hud_Hud_SetRadarPos(JNIEnv *env, jobject thiz,
        jfloat x1, jfloat y1, jfloat size) {
CHud::radarPos.x = x1;
CHud::radarPos.y = y1;
CHud::radarSize = size * 1.0f;
}

extern "C"
JNIEXPORT void JNICALL Java_com_goldrp_game_ui_hud_Hud_nativeBoolShowHud(JNIEnv *env, jobject thiz, jboolean toggle) {
CHud::bIsShow = toggle;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_goldrp_game_ui_hud_Hud_nativeClick(JNIEnv *env, jobject thiz, jint buttonId) {
switch (buttonId) {
case CHud::BUTTON_MENU: {
if (pNetGame) {
pNetGame->SendChatCommand("/mm");
}
break;
}
case CHud::BUTTON_STAR: {
if (pNetGame) {
pNetGame->SendChatCommand("/bkuzn");
}
break;
}
case CHud::BUTTON_INV: {
if (pNetGame) {
pNetGame->SendChatCommand("/inv");
}
break;
}
case CHud::BUTTON_SHOP: {
if (pNetGame) {
pNetGame->SendChatCommand("/donate");
}
break;
}
case CHud::BUTTON_HELP: {
if (pNetGame) {
pNetGame->SendChatCommand("/help");
}
break;
}
}
}