#pragma once

#define DEBUG_GUI 0

#include "../vendor/encoding/encoding.h"
#include "imguiwrapper.h"
#include "uisettings.h"
#include "widget.h"
#include "widgets/layout.h"
#include "widgets/label.h"
#include "widgets/button.h"
#include "widgets/image.h"
#include "widgets/progressbar.h"
#include "widgets/scrollpanel.h"
#include "widgets/listbox.h"
#include "widgets/editbox.h"
#include "samp_widgets/keyboard.h"
#include "samp_widgets/chat.h"
#include "samp_widgets/playertablist.h"
#include "samp_widgets/voicebutton.h"
#include "samp_widgets/dialogs/dialog.h"
#include "../vendor/raknet/SingleProducerConsumer.h"

#pragma pack(push, 1)
struct BUFFERED_COMMAND_TEXTDRAW
{
    uint16_t textdrawId;
};
#pragma pack(pop)

enum eTouchType
{
    TOUCH_POP = 1,
    TOUCH_PUSH = 2,
    TOUCH_MOVE = 3
};

class Keyboard;
class Chat;
class Dialog;
class PlayerTabList;
class VoiceButton;
class Label;

class UI : public Widget, public ImGuiWrapper
{
public:
    UI(const ImVec2& display_size, const std::string& font_path);

    bool initialize() override;
    void render() override;
    void shutdown() override;

    static inline bool m_bCamEditGui = false;

    Chat* chat() const { return m_chat; }
    Keyboard* keyboard() const { return m_keyboard; }
    Dialog* dialog() const { return m_dialog; }
    PlayerTabList* playertablist() const { return m_playerTabList; }
    VoiceButton* voicebutton() const { return m_voiceButton; }

    static const ImColor fixcolor(uint32_t color) {
        return ImColor(
                (int)((color & 0xFF000000) >> 24),
                (int)((color & 0x00FF0000) >> 16),
                (int)((color & 0x0000FF00) >> 8));
    }

    float ScaleX(float x) {
        return x * displaySize().x * (1.0f / 1920.0f);
    }

    float ScaleY(float y) {
        return y * displaySize().y * (1.0f / 1080.0f);
    }

    float GetFontSize() {
        return UISettings::fontSize();
    }

    void loadTxd();

    static inline struct RwTexture* m_pPassengerButtonTex;

    virtual void touchEvent(const ImVec2& pos, TouchType type) override;

    bool OnTouchEvent(int type, bool multi, int x, int y);

    void renderDebug();

    void ProcessPushedTextdraws();
    void PushToBufferedQueueTextDrawPressed(uint16_t textdrawId);

protected:
    void drawList() override;

private:
    Keyboard* m_keyboard;
    Chat* m_chat;
    Dialog* m_dialog;
    PlayerTabList* m_playerTabList;
    VoiceButton* m_voiceButton;
    Label* label;
    Label* label2;
    Label* label3;
    Label* label4;
    Label* label5;
    Label* label6;
    Label* label7;
    Label* label8;

    float m_fFontSize;
    bool m_bNeedClearMousePos = false;

    DataStructures::SingleProducerConsumer<BUFFERED_COMMAND_TEXTDRAW> m_BufferedCommandTextdraws;
};