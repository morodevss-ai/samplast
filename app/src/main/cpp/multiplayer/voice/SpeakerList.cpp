#include "main.h"
#include "game/game.h"
#include "game/playerped.h"
#include "net/netgame.h"

#include "SpeakerList.h"

#include "PluginConfig.h"

extern UI *pUI;
extern CNetGame *pNetGame;

bool SpeakerList::Init() noexcept
{
    if(SpeakerList::initStatus)
        return false;

    try
    {
        SpeakerList::tSpeakerIcon = CUtil::LoadTextureFromDB("samp", "speaker");
    }
    catch (const std::exception& exception)
    {
        // .. LogVoice("[sv:err:speakerlist:init] : failed to create speaker icon");
        SpeakerList::tSpeakerIcon = nullptr;
        return false;
    }

    if(!PluginConfig::IsSpeakerLoaded())
    {
        PluginConfig::SetSpeakerLoaded(true);
        SpeakerList::ResetConfigs();
    }

    SpeakerList::initStatus = true;
    SpeakerList::SyncConfigs();

    return true;
}

void SpeakerList::Free() noexcept
{
    if(!SpeakerList::initStatus)
        return;

    SpeakerList::tSpeakerIcon = nullptr;

    SpeakerList::initStatus = false;
}

void SpeakerList::Show() noexcept
{
    SpeakerList::showStatus = true;
}

bool SpeakerList::IsShowed() noexcept
{
    return SpeakerList::showStatus;
}

void SpeakerList::Hide() noexcept
{
    SpeakerList::showStatus = false;
}

void SpeakerList::Render()
{
    if(!SpeakerList::initStatus || !SpeakerList::IsShowed())
        return;

    if(!pNetGame) return;

    CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
    if(!pPlayerPool) return;

    int curTextLine;
    char szText[256], szText2[256];
    ImVec2 textPos = ImVec2(pUI->ScaleX(24), pUI->ScaleY(480));

    for(PLAYERID playerId { 0 }; playerId < MAX_PLAYERS; ++playerId)
    {
        CRemotePlayer* pPlayer = pPlayerPool->GetAt(playerId);
        if(pPlayer && pPlayer->IsActive())
        {
            CPlayerPed* pPlayerPed = pPlayer->GetPlayerPed();
            if(pPlayerPed)
            {
                if(const auto playerName = pPlayerPool->GetPlayerName(playerId); playerName)
                {
                    if(!SpeakerList::playerStreams[playerId].empty())
                    {
                        for(const auto& playerStream : SpeakerList::playerStreams[playerId])
                        {
                            if(playerStream.second.GetType() != StreamType::GlobalStream)
                            {
                                if(pPlayerPed->m_pPed->GetDistanceFromLocalPlayerPed() > playerStream.second.GetDistance())
                                    SpeakerList::OnSpeakerStop(*SpeakerList::playerStream[playerId], playerId);
                            }

                            if(playerStream.second.GetType() == StreamType::LocalStreamAtPlayer)
                            {
                                CVector VecPos;

                                if(!pPlayerPed->m_pPed->IsAdded()) continue;
                                VecPos.x = 0.0f;
                                VecPos.y = 0.0f;
                                VecPos.z = 0.0f;
                                pPlayerPed->GetBonePosition(8, &VecPos);

                                CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
                                int dwHitEntity = 0;

                                dwHitEntity = ScriptCommand(&get_line_of_sight, VecPos.x, VecPos.y, VecPos.z,
                                    TheCamera.GetPosition().x, TheCamera.GetPosition().y, TheCamera.GetPosition().z, 1, 0, 0, 1, 0);

                                if(dwHitEntity && pPlayerPed->m_pPed->GetDistanceFromLocalPlayerPed() <= 40.0f)
                                    SpeakerList::Draw(&VecPos, pPlayerPed->m_pPed->GetDistanceFromCamera());
                            }
                        }

                        if(curTextLine < 8)
                        {
                            ImVec2 a = ImVec2(textPos.x, textPos.y);
                            ImVec2 b = ImVec2(textPos.x + pUI->GetFontSize(), textPos.y + pUI->GetFontSize());
                            ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)SpeakerList::tSpeakerIcon->raster, a, b);

                            float fSpaceFromIconToText = pUI->GetFontSize() + ImGui::GetStyle().ItemSpacing.x;

                            textPos.x += fSpaceFromIconToText;
                            sprintf(szText, "%s (%d) ", playerName, playerId);
                            pUI->renderer()->drawText(textPos, 0xFFFFFFFF, szText, true,
                                                       UISettings::fontSize() / 2);

                            for(const auto& streamInfo : SpeakerList::playerStreams[playerId])
                            {
                                if(streamInfo.second.GetColor() == NULL)
                                    continue;

                                textPos.x += ImGui::CalcTextSize(szText).x;
                                sprintf(szText2, "[%s]", streamInfo.second.GetName().c_str());
                                pUI->renderer()->drawText(textPos, streamInfo.second.GetColor(),
                                                          szText2, true, UISettings::fontSize() / 2);
                            }

                            textPos.x -= fSpaceFromIconToText;
                            textPos.x -= ImGui::CalcTextSize(szText).x;
                            textPos.y += pUI->GetFontSize();

                            curTextLine++;
                        }
                    }
                }
            }
        }
    }
}

void SpeakerList::Draw(CVector* vec, float fDist)
{
    CVector TagPos;

	TagPos.x = vec->x;
	TagPos.y = vec->y;
	TagPos.z = vec->z;
	TagPos.z += 0.25f + (fDist * 0.0475f);

    CVector Out;
	// CSprite::CalcScreenCoors
	((void (*)(CVector*, CVector*, float*, float*, bool, bool))(g_libGTASA + (VER_x32 ? 0x005C57E8 + 1 : 0x6E9DF8)))(&TagPos, &Out, 0, 0, 0, 0);

	if(Out.z < 1.0f)
		return;

    ImVec2 pos = ImVec2(Out.x, Out.y);
	pos.x -= SpeakerList::GetSpeakerIconScale() / 2;
    pos.y -= pUI->GetFontSize() * 1.5;

    ImVec2 a = ImVec2(pos.x, pos.y);
    ImVec2 b = ImVec2(pos.x + SpeakerList::GetSpeakerIconScale(), pos.y + SpeakerList::GetSpeakerIconScale());
    ImGui::GetBackgroundDrawList()->AddImage((ImTextureID)SpeakerList::tSpeakerIcon->raster, a, b);
}

void SpeakerList::OnSpeakerPlay(const Stream& stream, const uint16_t speaker) noexcept
{
    uint16_t wSpeaker = speaker;
    if(speaker < 0) wSpeaker = 0;
    else if(speaker > MAX_PLAYERS - 1) wSpeaker = MAX_PLAYERS - 1;
    if(speaker != wSpeaker) return;

    SpeakerList::playerStream[speaker] = &stream;
    SpeakerList::playerStreams[speaker][(Stream*)(&stream)] = stream.GetInfo();
}

void SpeakerList::OnSpeakerStop(const Stream& stream, const uint16_t speaker) noexcept
{
    uint16_t wSpeaker = speaker;
    if(speaker < 0) wSpeaker = 0;
    else if(speaker > MAX_PLAYERS - 1) wSpeaker = MAX_PLAYERS - 1;
    if(speaker != wSpeaker) return;

    SpeakerList::playerStreams[speaker].erase((Stream*)(&stream));
}

float SpeakerList::GetSpeakerIconScale() noexcept
{
    return PluginConfig::GetSpeakerIconScale();
}

void SpeakerList::SetSpeakerIconScale(const float speakerIconScale) noexcept
{
    float iSpeakerIconScale = speakerIconScale;
    if(speakerIconScale < 20.f) iSpeakerIconScale = 20.f;
    else if(speakerIconScale > 60.f) iSpeakerIconScale = 60.f;

    PluginConfig::SetSpeakerIconScale(iSpeakerIconScale);
}

void SpeakerList::SyncConfigs() noexcept
{
    SpeakerList::SetSpeakerIconScale(PluginConfig::GetSpeakerIconScale());
}

void SpeakerList::ResetConfigs() noexcept
{
    PluginConfig::SetSpeakerIconScale(PluginConfig::kSpeakerIconSize);
}

std::array<std::unordered_map<Stream*, StreamInfo>, MAX_PLAYERS> SpeakerList::playerStreams;
std::array<const Stream*, MAX_PLAYERS> SpeakerList::playerStream;

bool SpeakerList::initStatus { false };
bool SpeakerList::showStatus { false };

RwTexture* SpeakerList::tSpeakerIcon { nullptr };