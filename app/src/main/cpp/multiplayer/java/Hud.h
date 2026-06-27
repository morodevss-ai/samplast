#ifndef SA_MP_LAUNCHER_CHUD_H
#define SA_MP_LAUNCHER_CHUD_H

#include <jni.h>
#include "game/core/Vector2D.h"

class CHud {
public:
    static jobject thiz;
    static bool bIsShow;
    static int iWantedLevel;
    static int iLocalMoney;
    static CVector2D radarBgPos1;
    static CVector2D radarBgPos2;
    static CVector2D radarPos;
    static float radarSize;

    static const int BUTTON_MENU = 0;
    static const int BUTTON_STAR = 1;
    static const int BUTTON_INV = 2;
    static const int BUTTON_SHOP = 3;
    static const int BUTTON_HELP = 4;

    static bool bIsShowEnterExitButt;
    static void toggleAll(bool toggle, bool isChat);
    static void UpdateHudInfo();
    static void UpdateWanted();
    static void UpdateMoney();
    static void UpdateAmmo();
    static void ForceUpdateAmmo();
    static void toggleLogo(bool toggle);
    static void updatePlayerInfo(int id, char *name);

    static bool NeededRenderPassengerButton();
};

#endif