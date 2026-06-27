#define IMGUI_DEFINE_MATH_OPERATORS
#include "../vendor/imgui/imgui.h"
#include "../vendor/imgui/imgui_internal.h"

#include <string>
#include "uisettings.h"
#include "../main.h"
#include "../settings.h"

extern CSettings* pSettings;

/* scaling */
ImVec2 UISettings::m_baseSize = ImVec2(640.0f, 480.0f);
ImVec2 UISettings::m_scaleFactor = ImVec2(1.0f, 1.0f);

/* font */
float UISettings::m_fontSize = 20.0f;//26.0f;//24.0f;
float UISettings::m_outlineSize = 2.0f;

/* ////////////////// pos & size ////////////////// */

/* widgets */
float UISettings::m_padding = 15.0f;

ImVec2 UISettings::m_keyboardPos = ImVec2(0.0f, 223.0f/*180.0f*/);
ImVec2 UISettings::m_keyboardSize = ImVec2(640.0f, 300.0f);//ImVec2(640.0f, 300.0f);
float UISettings::m_keyboardRowHeight = 52.0f;//52.0f;

/* chat */
int UISettings::m_chatDispMesssages = 9;
int UISettings::m_chatMaxMessages = 30;
ImVec2 UISettings::m_chatPos = ImVec2(114.0f, 14.0f);
ImVec2 UISettings::m_chatTabletPos = ImVec2(113.0f, 18.0f);
ImVec2 UISettings::m_chatSize = ImVec2(400.0f, 0.0f);
ImVec2 UISettings::m_chatItemSize = ImVec2(400.0f, 13.0f);

/* nametag */
ImVec2 UISettings::m_nametagBarSize = ImVec2(25.0f, 6.0f);

/* dialog */
ImVec2 UISettings::m_dialogButtonPanelSize = ImVec2(150.0f, 45.0f);
ImVec2 UISettings::m_dialogMinSize = ImVec2(150.0f, 120.0f);
ImVec2 UISettings::m_dialogMaxSize = ImVec2(620.0f, 400.0f);
float UISettings::m_dialogTitleHeight = 20.0f;
float UISettings::m_dialogListItemHeight = 30.0f;

/* voice button */
ImVec2 UISettings::m_buttonVoicePos = ImVec2(500.0f, 180.0f/*170.0f*/);
ImVec2 UISettings::m_buttonVoiceSize = ImVec2(55.0f, 80.0f);

/* ////////////////// colors ////////////////// */

/* button */
// Glass-like buttons: base semi-transparent gray, focused a bit brighter
ImColor UISettings::m_buttonColor = ImColor(0.0f, 0.0f, 0.0f, 0.0f);
ImColor UISettings::m_buttonFocusedColor = ImColor(0.0f, 0.0f, 0.0f, 0.0f);

/* keyboard */
ImColor UISettings::m_keyboardBackgroundColor = ImColor(0, 0, 0, 150);

/* dialog */
ImColor UISettings::m_dialogBackgroundColor = ImColor(0, 0, 0, 200);
ImColor UISettings::m_dialogTitleBackgroundColor = ImColor(0, 0, 0, 200);/*ImColor(0xF5, 0x91, 0x32);*/// ImColor(50, 50, 50, 255);

void UISettings::Initialize(const ImVec2& display_size)
{
    if(!pSettings) return;

    m_scaleFactor = display_size / m_baseSize;

    /* chat line count */
    m_chatDispMesssages = pSettings->Get().iChatMaxMessages;

    /* font */
    m_fontSize *= m_scaleFactor.y;

    /* keyboard */
    m_keyboardSize = m_keyboardSize * m_scaleFactor;
    m_keyboardPos = m_keyboardPos * m_scaleFactor;
    m_keyboardRowHeight *= m_scaleFactor.y;

    /* chat */
    m_chatPos = m_chatPos * m_scaleFactor;
    m_chatTabletPos = m_chatTabletPos * m_scaleFactor;
    m_chatSize.y = m_chatItemSize.y * m_chatDispMesssages;
    m_chatSize = m_chatSize * m_scaleFactor;
    m_chatItemSize = m_chatItemSize * m_scaleFactor;

    /* nametag */
    m_nametagBarSize = m_nametagBarSize * m_scaleFactor;

    /* dialog */
    m_dialogButtonPanelSize = m_dialogButtonPanelSize * m_scaleFactor;
    m_dialogMinSize = m_dialogMinSize * m_scaleFactor;
    m_dialogMaxSize = m_dialogMaxSize * m_scaleFactor;
    m_dialogTitleHeight *= m_scaleFactor.y;
    m_dialogListItemHeight *= m_scaleFactor.y;

    /* button voice */
    m_buttonVoicePos = m_buttonVoicePos * m_scaleFactor;
    m_buttonVoiceSize = m_buttonVoiceSize * m_scaleFactor;
}