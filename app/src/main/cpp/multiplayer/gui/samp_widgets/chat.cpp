#include "../gui.h"
#include "../../main.h"
#include "../../game/game.h"
#include "../../net/netgame.h"
#include <algorithm>
#include "../settings.h"
#include "java/jniutil.h"
#include <cmath>
#include <sstream>
#include "java/editobject.h"

extern UI* pUI;
extern CGame* pGame;
extern CNetGame* pNetGame;
extern CSettings* pSettings;
extern CJavaWrapper *pJavaWrapper;

Chat::Chat()
        : ListBox()
{
    m_shouldAutoScroll = true;
}

void Chat::addChatMessage(const std::string& message, const std::string& nick, const ImColor& nick_color)
{
    addPlayerMessage(message, nick, nick_color);
}

void Chat::addInfoMessage(const std::string& format, ...)
{
    char tmp_buf[512];

    va_list args;
    va_start(args, format);
    vsprintf(tmp_buf, format.c_str(), args);
    va_end(args);

    addMessage(std::string(tmp_buf), ImColor(0x00, 0xc8, 0xc8));
}

void Chat::addDebugMessage(const std::string& format, ...)
{
    char tmp_buf[512];

    va_list args;
    va_start(args, format);
    vsprintf(tmp_buf, format.c_str(), args);
    va_end(args);

    addMessage(std::string(tmp_buf), ImColor(0xbe, 0xbe, 0xbe));
}

void Chat::addClientMessage(const std::string& message, const ImColor& color)
{
    addMessage(message, color);
}

void Chat::addMessage(const std::string& message, const ImColor& color)
{
    if (this->itemsCount() > UISettings::chatMaxMessages())
    {
        this->removeItem(0);
    }

    MessageItem* item = new MessageItem(message, color);
    this->addItem(item);

    if (m_shouldAutoScroll)
    {
        this->setScrollY(1.0f);
    }
}

void Chat::addPlayerMessage(const std::string& message, const std::string& nick, const ImColor& nick_color)
{
    if (this->itemsCount() > UISettings::chatMaxMessages())
    {
        this->removeItem(0);
    }

    PlayerMessageItem* item = new PlayerMessageItem(message, nick, nick_color);
    this->addItem(item);

    if (m_shouldAutoScroll)
    {
        this->setScrollY(1.0f);
    }
}

ImVec2 Chat::getAdaptiveChatPosition()
{
    if (!RsGlobal) return UISettings::chatPos();

    float width = static_cast<float>(RsGlobal->maximumWidth);
    float height = static_cast<float>(RsGlobal->maximumHeight);

    float minSide = fmin(width, height);
    float maxSide = fmax(width, height);
    float aspectRatio = maxSide / minSide;

    if (aspectRatio < 1.7f && minSide > 700.0f) {
        return UISettings::chatTabletPos();
    } else {
        return UISettings::chatPos();
    }
}

void Chat::draw(ImGuiRenderer* renderer)
{
    ImVec2 chatPosition = getAdaptiveChatPosition();
    this->setPosition(chatPosition);

    ListBox::draw(renderer);
}

void Chat::activateEvent(bool active)
{
    if (active)
    {
        this->setScrollable(true);
    }
    else
    {
        this->setScrollable(false);
        m_shouldAutoScroll = true;
    }
}

void Chat::touchPopEvent()
{
    if (pUI->playertablist()->visible()) return;

    m_shouldAutoScroll = false;
    toggleKeyboard();
}

void Chat::toggleKeyboard()
{
    if (m_keyboardActive)
    {
        pUI->keyboard()->hide();
        setKeyboardActive(false);
    }
    else
    {
        pUI->keyboard()->show(this);
        setKeyboardActive(true);
    }
}

void Chat::startEditAttach(int slot)
{
    if (!pNetGame) {
        pUI->chat()->addDebugMessage("No netgame");
        return;
    }

    if (!pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed()) {
        pUI->chat()->addDebugMessage("No local player");
        return;
    }

    auto pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();

    if (!pPlayer->GetAttachedObject(slot)) {
        pUI->chat()->addDebugMessage("No attached object in slot %d", slot);
        return;
    }

    CObjectEditor::startEditPlayerAttach(slot);
    pUI->chat()->addInfoMessage("Started editing attach slot %d", slot);
}

bool Chat::commandClient(const std::string& command)
{
    if (command == "/dl")
    {
        CGame::m_bDl_enabled = !CGame::m_bDl_enabled;
        return true;
    }

    if (command == "/tab")
    {
        static int Tab = 0;

        if (Tab == 0) {
            ShowJavaTabList();
            Tab = 1;
        }
        else {
            HideJavaTabList();
            Tab = 0;
        }
        return true;
    }

    if (command == "/testattach")
    {
        if (!pNetGame) {
            pUI->chat()->addDebugMessage("Not connected to server");
            return true;
        }

        auto pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
        if (!pPlayer) {
            pUI->chat()->addDebugMessage("No local player");
            return true;
        }

        int foundSlot = -1;
        for (int i = 0; i < 10; i++) {
            if (pPlayer->GetAttachedObject(i)) {
                foundSlot = i;
                break;
            }
        }

        if (foundSlot != -1) {
            Chat::startEditAttach(foundSlot);
            pUI->chat()->addInfoMessage("Editing attach slot %d", foundSlot);
        } else {
            pUI->chat()->addInfoMessage("No attached objects found");
        }
        return true;
    }

    if (command == "/reconnect")
    {
        if (pNetGame->GetGameState() == GAMESTATE_CONNECTED)
        {
            pNetGame->ShutdownForGameModeRestart();
        }
        else
        {
            pNetGame->SetGameState(GAMESTATE_WAIT_CONNECT);
        }
        return true;
    }

    return false;
}

void Chat::keyboardEvent(const std::string& input)
{
    if (input.length() > 0)
    {
        if (input[0] == '/' && commandClient(input))
        {
            if (m_keyboardActive)
            {
                pUI->keyboard()->hide();
                setKeyboardActive(false);
            }
            return;
        }

        if (pNetGame)
        {
            if (input[0] == '/') pNetGame->SendChatCommand(input.c_str());
            else pNetGame->SendChatMessage(input.c_str());
        }
    }

    if (m_keyboardActive)
    {
        pUI->keyboard()->hide();
        setKeyboardActive(false);
        m_shouldAutoScroll = true;
        this->setScrollY(1.0f);
    }
}

std::string Chat::sDevTagNick = "";