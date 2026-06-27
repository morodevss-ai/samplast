#include "../main.h"
#include "../game/game.h"
#include "../net/netgame.h"
#include "gui.h"
#include "PlayerTags.h"
#include "../net/playerbubblepool.h"
#include "vendor/str_obfuscator/str_obfuscator.hpp"
#include "../voice/Plugin.h"
#include "../voice/MicroIcon.h"
#include "../voice/SpeakerList.h"
#include <chrono>
#include "../voice/Network.h"
#include "../gui/samp_widgets/voicebutton.h"
#include "game/Textures/TextureDatabaseRuntime.h"
#include "../game/Streaming.h"
#include "../game/Pools.h"
#include "../java/jniutil.h"
#include "../settings.h"
#include "../vendor/raknet/RakNetStatistics.h"
#include "game/Mobile/MobileMenu/MobileMenu.h"
#include "java/Speedometr.h"
#include "java/Hud.h"
#include "game/Widgets/TouchInterface.h"

extern CNetGame* pNetGame;
extern CPlayerTags* pPlayerTags;
extern UI* pUI;
class CJavaWrapper;
extern CJavaWrapper* pJavaWrapper;
extern CMobileMenu* gMobileMenu;
extern CGame *pGame;
extern CSettings* pSettings;
extern bool MobileMenuBool;
extern bool m_bKeyboardOpened;

UI::UI(const ImVec2& display_size, const std::string& font_path)
        : Widget(), ImGuiWrapper(display_size, font_path)
{
    UISettings::Initialize(display_size);
    this->setFixedSize(display_size);
}

bool UI::initialize()
{
    if (!ImGuiWrapper::initialize()) return false;

    m_chat = new Chat();
    this->addChild(m_chat);
    m_chat->setFixedSize(UISettings::chatSize());
    m_chat->setPosition(UISettings::chatPos());
    m_chat->setItemSize(UISettings::chatItemSize());
    m_chat->setVisible(false);

    m_voiceButton = new VoiceButton();
    this->addChild(m_voiceButton);
    m_voiceButton->setFixedSize(UISettings::buttonVoiceSize());
    m_voiceButton->setPosition(UISettings::buttonVoicePos());
    m_voiceButton->setVisible(false);

    m_dialog = new Dialog();
    this->addChild(m_dialog);
    m_dialog->setVisible(false);
    m_dialog->setMinSize(UISettings::dialogMinSize());
    m_dialog->setMaxSize(UISettings::dialogMaxSize());

    m_keyboard = new Keyboard();
    this->addChild(m_keyboard);
    m_keyboard->setFixedSize(UISettings::keyboardSize());
    m_keyboard->setPosition(UISettings::keyboardPos());
    m_keyboard->setVisible(false);

    m_playerTabList = new PlayerTabList();
    this->addChild(m_playerTabList);
    m_playerTabList->setMinSize(UISettings::dialogMinSize());
    m_playerTabList->setMaxSize(UISettings::dialogMaxSize());
    m_playerTabList->setVisible(false);

    label = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label);

    label2 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label2);

    label3 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label3);

    label4 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label4);

    label5 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label5);

    label6 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label6);

    label7 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label7);

    label8 = new Label(" ", ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
    pUI->addChild(label8);

    Label* d_label1;
    if(VER_x32)
    {
        d_label1 = new Label(cryptor::create("Gold Russia x32").decrypt(), ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
        this->addChild(d_label1);
        d_label1->setPosition(ImVec2(3.0, 3.0));
    }
    else
    {
        d_label1 = new Label(cryptor::create("Gold Russia (arm64-v8a)").decrypt(), ImColor(1.0f, 1.0f, 1.0f), true, UISettings::fontSize() / 2);
        this->addChild(d_label1);
        d_label1->setPosition(ImVec2(3.0, 3.0));
    }

    return true;
}

void UI::loadTxd() {
    FLog("Load gui textures..");
    m_pPassengerButtonTex = CUtil::LoadTextureFromDB("gui", OBFUSCATE("gtexture"));
}
void RenderBackgroundHud();
void UI::render()
{
    ImGuiWrapper::render();

    if(pNetGame && pNetGame->GetPlayerPool() && pNetGame->GetPlayerPool()->GetLocalPlayer())
    {
        bool isPlayerActive = pNetGame->GetPlayerPool()->GetLocalPlayer()->m_bIsActive;
        if(m_voiceButton->visible() != isPlayerActive)
        {
            m_voiceButton->setVisible(isPlayerActive);
        }
    }
   // renderDebug();
   // RenderPassengerButton();

    if (pGame->FindPlayerPed()->IsInVehicle() && !pGame->FindPlayerPed()->IsAPassenger()) {
        if (!CSpeedometr::bIsShow && !MobileMenuBool && !m_bKeyboardOpened) {
            CSpeedometr::show();
        }
        if (!MobileMenuBool && !m_bKeyboardOpened) {
            CSpeedometr::update();
        }
    } else {
        if (CSpeedometr::bIsShow) {
            CSpeedometr::hide();
        }
    }

    ProcessPushedTextdraws();

    if (m_bNeedClearMousePos) {
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(-1, -1);
        m_bNeedClearMousePos = false;
    }
}

void UI::shutdown()
{
    ImGuiWrapper::shutdown();
}

void UI::drawList()
{
    if (!visible()) return;

    if (pPlayerTags) pPlayerTags->Render(renderer());
    if (pNetGame && pNetGame->GetTextLabelPool()) pNetGame->GetTextLabelPool()->Render(renderer());
    if (pNetGame && pNetGame->GetPlayerBubblePool()) pNetGame->GetPlayerBubblePool()->Render(renderer());

    draw(renderer());
}

void UI::touchEvent(const ImVec2& pos, TouchType type)
{
    if(!visible()) return;

    if (m_keyboard->visible() && m_keyboard->contains(pos))
    {
        m_keyboard->touchEvent(pos, type);
        return;
    }

    if (m_dialog->visible() && m_dialog->contains(pos))
    {
        m_dialog->touchEvent(pos, type);
        return;
    }

    Widget::touchEvent(pos, type);
}

bool UI::OnTouchEvent(int type, bool multi, int x, int y)
{
    ImGuiIO& io = ImGui::GetIO();

    VoiceButton* vbutton = pUI->voicebutton();
    switch (type)
    {
        case TOUCH_PUSH:
            io.MousePos = ImVec2(x, y);
            io.MouseDown[0] = true;
            break;

        case TOUCH_POP:
            io.MouseDown[0] = false;
            m_bNeedClearMousePos = true;
            break;

        case TOUCH_MOVE:
            io.MousePos = ImVec2(x, y);
            break;
    }

    return true;
}

void UI::renderDebug()
{
    if(!pSettings->Get().iFPSCounter) return;

    char szStr[30];
    char szStrMem[64];
    char szStrPos[64];

    float yOffset = 50.0f;

    ImVec2 pos = ImVec2(pUI->ScaleX(40.0f), pUI->ScaleY(1080.0f - UISettings::fontSize() * 4.5));

    static float fps = 120.f;
    static auto lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
    if(CTimer::m_snTimeInMillisecondsNonClipped - lastTick > 500) {
        lastTick = CTimer::m_snTimeInMillisecondsNonClipped;
        fps = std::clamp(CTimer::game_FPS, 10.f, (float) 120);
    }
    if(VER_x32)
    {
        snprintf(&szStr[0], sizeof(szStr), "x32 FPS: %.0f", fps);
    }
    else
    {
        snprintf(&szStr[0], sizeof(szStr), "x64 FPS: %.0f", fps);
    }

    label->setText(&szStr[0]);
    label->setPosition(pos);

    auto &msUsed = CStreaming::ms_memoryUsed;
    auto &msAvailable = CStreaming::ms_memoryAvailable;

    struct mallinfo memInfo = mallinfo();
    int totalAllocatedMB  = memInfo.uordblks / (1024 * 1024);

    snprintf(&szStrMem[0], sizeof(szStrMem), "MEM: %d mb (stream %d/%d) (Tex %d MB)",
             totalAllocatedMB,
             msUsed / (1024 * 1024),
             msAvailable / (1024 * 1024),
             TextureDatabaseRuntime::storedTexels / (1024 * 1024)
    );

    if (totalAllocatedMB >= 600)
    {
        CStreaming::MakeSpaceFor(500);
    }

    pos = ImVec2(pUI->ScaleX(40.0f), pUI->ScaleY(1080.0f - UISettings::fontSize() * 4));

    label2->setText(&szStrMem[0]);
    label2->setPosition(pos);

    if (pGame->FindPlayerPed()->m_pPed)
    {
        snprintf(&szStrPos[0], sizeof(szStrPos), "POS: %.2f, %.2f, %.2f", pGame->FindPlayerPed()->m_pPed->m_matrix->m_pos.x, pGame->FindPlayerPed()->m_pPed->m_matrix->m_pos.y, pGame->FindPlayerPed()->m_pPed->m_matrix->m_pos.z);
        pos = ImVec2(pUI->ScaleX(40.0f), pUI->ScaleY(1080.0f - UISettings::fontSize() * 3.5));
        label3->setText(&szStrPos[0]);
        label3->setPosition(pos);
    }

    char debugPools[250];
    snprintf(&debugPools[0], sizeof(debugPools), "NSingle: %d/100000; NDouble: %d/60000; Peds: %d/240; Veh's: %d/1000; Obj: %d/3000; EntryInf: %d/60000; Dummies: %d/80000, Buildings: %d/60000",
             GetPtrNodeSingleLinkPool()->GetNoOfUsedSpaces(),
             GetPtrNodeDoubleLinkPool()->GetNoOfUsedSpaces(),
             GetPedPoolGta()->GetNoOfUsedSpaces(),
             GetVehiclePoolGta()->GetNoOfUsedSpaces(),
             GetObjectPoolGta()->GetNoOfUsedSpaces(),
             GetEntryInfoNodePool()->GetNoOfUsedSpaces(),
             GetDummyPool()->GetNoOfUsedSpaces(),
             GetBuildingPool()->GetNoOfUsedSpaces()
    );

    pos = ImVec2(pUI->ScaleX(40.0f), pUI->ScaleY(1080.0f - UISettings::fontSize() * 0.5));
    label4->setText(&debugPools[0]);
    label4->setPosition(pos);
}
/*
void UI::RenderPassengerButton()
{
    if(!CHud::NeededRenderPassengerButton())
        return;

    if(!m_pPassengerButtonTex)
    {
        m_pPassengerButtonTex = CUtil::LoadTextureFromDB("samp", "gbutton");
        if(!m_pPassengerButtonTex)
            return;
    }

    auto& driverButton = CTouchInterface::m_pWidgets[WidgetIDs::WIDGET_ENTER_CAR];
    if(!driverButton) return;

    float driverButtonSize = driverButton->m_RectScreen.right - driverButton->m_RectScreen.left;

    ImVec2 butSize = ImVec2(driverButtonSize, driverButtonSize);
    ImVec2 butPos = ImVec2(driverButton->m_RectScreen.left - driverButtonSize - 10.0f, driverButton->m_RectScreen.bottom - driverButtonSize);

    ImGuiIO& io = ImGui::GetIO();
    if(butPos.x < 0) butPos.x = 10.0f;
    if(butPos.y < 0) butPos.y = 10.0f;
    if(butPos.x + butSize.x > io.DisplaySize.x) butPos.x = io.DisplaySize.x - butSize.x - 10.0f;
    if(butPos.y + butSize.y > io.DisplaySize.y) butPos.y = io.DisplaySize.y - butSize.y - 10.0f;

    ImGui::SetNextWindowPos(butPos);
    ImGui::SetNextWindowSize(butSize);

    ImGui::Begin("passenger_button", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
    {
        ImGui::SetCursorPos(ImVec2(0, 0));

        if (ImGui::ImageButton((ImTextureID) m_pPassengerButtonTex->raster, butSize)) {
            //CLocalPlayer::GoEnterVehicle(true);
        }
    }
    ImGui::End();
}
*/
void UI::PushToBufferedQueueTextDrawPressed(uint16_t textdrawId)
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = m_BufferedCommandTextdraws.WriteLock();

    pCmd->textdrawId = textdrawId;

    m_BufferedCommandTextdraws.WriteUnlock();
}

void UI::ProcessPushedTextdraws()
{
    BUFFERED_COMMAND_TEXTDRAW* pCmd = nullptr;
    while (pCmd = m_BufferedCommandTextdraws.ReadLock())
    {
        RakNet::BitStream bs;
        bs.Write(pCmd->textdrawId);
        pNetGame->GetRakClient()->RPC(&RPC_ClickTextDraw, &bs, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false, UNASSIGNED_NETWORK_ID, 0);
        m_BufferedCommandTextdraws.ReadUnlock();
    }
}