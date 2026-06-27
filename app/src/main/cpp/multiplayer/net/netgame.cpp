#include "../main.h"
#include "../game/game.h"
#include "netgame.h"
#include "../gui/gui.h"
#include "AudioStream.h"

// voice
#include "../voice/MicroIcon.h"
#include "../voice/SpeakerList.h"
#include "../voice/Network.h"
#include "java/jniutil.h"
#include "java/Speedometr.h"
#include "util/CUtil.h"

#define AUTH_BS "39FB2DEEDB49ACFB8D4EECE6953D2507988CCCF4410"//main
//#define AUTH_BS "E02262CF28BC542486C558D4BE9EFB716592AFAF8B"

#define RPC_SHOW_NOTIFICATION 0x32

#define RPC_SHOW_SPAWN_SELECTOR_WINDOW  0x24

void cp1251_to_utf8(char* out, const char* in, unsigned int len)
{
    static const int table[128] =
            {
                    // 80
                    0x82D0,     0x83D0,     0x9A80E2,   0x93D1,     0x9E80E2,   0xA680E2,   0xA080E2,   0xA180E2,
                    0xAC82E2,   0xB080E2,   0x89D0,     0xB980E2,   0x8AD0,     0x8CD0,     0x8BD0,     0x8FD0,
                    // 90
                    0x92D1,     0x9880E2,   0x9980E2,   0x9C80E2,   0x9D80E2,   0xA280E2,   0x9380E2,   0x9480E2,
                    0,          0xA284E2,   0x99D1,     0xBA80E2,   0x9AD1,     0x9CD1,     0x9BD1,     0x9FD1,
                    // A0
                    0xA0C2,     0x8ED0,     0x9ED1,     0x88D0,     0xA4C2,     0x90D2,     0xA6C2,     0xA7C2,
                    0x81D0,     0xA9C2,     0x84D0,     0xABC2,     0xACC2,     0xADC2,     0xAEC2,     0x87D0,
                    // B0
                    0xB0C2,     0xB1C2,     0x86D0,     0x96D1,     0x91D2,     0xB5C2,     0xB6C2,     0xB7C2,
                    0x91D1,     0x9684E2,   0x94D1,     0xBBC2,     0x98D1,     0x85D0,     0x95D1,     0x97D1,
                    // C0
                    0x90D0,     0x91D0,     0x92D0,     0x93D0,     0x94D0,     0x95D0,     0x96D0,     0x97D0,
                    0x98D0,     0x99D0,     0x9AD0,     0x9BD0,     0x9CD0,     0x9DD0,     0x9ED0,     0x9FD0,
                    // D0
                    0xA0D0,     0xA1D0,     0xA2D0,     0xA3D0,     0xA4D0,     0xA5D0,     0xA6D0,     0xA7D0,
                    0xA8D0,     0xA9D0,     0xAAD0,     0xABD0,     0xACD0,     0xADD0,     0xAED0,     0xAFD0,
                    // E0
                    0xB0D0,     0xB1D0,     0xB2D0,     0xB3D0,     0xB4D0,     0xB5D0,     0xB6D0,     0xB7D0,
                    0xB8D0,     0xB9D0,     0xBAD0,     0xBBD0,     0xBCD0,     0xBDD0,     0xBED0,     0xBFD0,
                    // F0
                    0x80D1,     0x81D1,     0x82D1,     0x83D1,     0x84D1,     0x85D1,     0x86D1,     0x87D1,
                    0x88D1,     0x89D1,     0x8AD1,     0x8BD1,     0x8CD1,     0x8DD1,     0x8ED1,     0x8FD1
            };

    int count = 0;

    while (*in)
    {
        if (len && (count >= len)) break;

        if (*in & 0x80)
        {
            int v = table[(int)(0x7f & *in++)];
            if (!v)
                continue;
            *out++ = (char)v;
            *out++ = (char)(v >> 8);
            if (v >>= 16)
                * out++ = (char)v;
        }
        else
            *out++ = *in++;

        count++;
    }

    *out = 0;
}

extern UI* pUI;
extern CGame* pGame;
extern CAudioStream* pAudioStream;
//extern CVoice* pVoice;

int iNetModeNormalOnFootSendRate = 30;
int iNetModeNormalInCarSendRate = 30;
int iNetModeFiringSendRate = 30;
int iNetModeSendMultiplier = 2;

void RegisterRPCs(RakClientInterface *pRakClient);
void UnregisterRPCs(RakClientInterface *pRakClient);
void RegisterScriptRPCs(RakClientInterface *pRakClient);
void UnregisterScriptRPCs(RakClientInterface *pRakClient);

void InstallVehicleEngineLightPatches();

// 0.3.7
unsigned char GetPacketID(Packet *p)
{
    if (p == 0) return 255;

    if ((unsigned char)p->data[0] == ID_TIMESTAMP)
        return (unsigned char)p->data[sizeof(unsigned char) + sizeof(unsigned long)];
    else
        return (unsigned char)p->data[0];
}

extern CJavaWrapper *pJavaWrapper;

CNetGame::CNetGame(const char* szHostOrIp, int iPort, const char *szPlayerName, const char* szPass)
{
    FLog("CNetGame initializing..");

    // voice
    Network::OnRaknetConnect(szHostOrIp, iPort);

    //MyLog2("Voice connect %s:%d", szHostOrIp, iPort);
    //MyLog2("Voice connect %s:%d", szHostOrIp, iPort);
    //MyLog2("Voice connect %s:%d", szHostOrIp, iPort);

    m_pNetSet = new NET_SETTINGS;
    memset(m_szHostName, 0, 256);
    memset(m_szHostOrIp, 0, 256);

    strcpy(m_szHostName, "GR BKUZN");
    strncpy(m_szHostOrIp, szHostOrIp, sizeof(m_szHostOrIp));
    m_iPort = iPort;

    m_pRakClient = RakNetworkFactory::GetRakClientInterface();
    InitializePools();

    GetPlayerPool()->SetLocalPlayerName(szPlayerName);

    RegisterRPCs(m_pRakClient);
    RegisterScriptRPCs(m_pRakClient);
    m_pRakClient->SetPassword(szPass);

    memset(m_dwMapIcon, 0, sizeof(m_dwMapIcon));

    pGame->EnableClock(false);
    pGame->EnableZoneNames(false);

    m_pNetSet->iDeathDropMoney = 0;
    m_pNetSet->iSpawnsAvailable = 0;
    m_pNetSet->bNameTagLOS = 0;
    m_pNetSet->byteHoldTime = true;
    m_pNetSet->bUseCJWalk = 0;
    m_pNetSet->bDisableInteriorEnterExits = 0;
    m_pNetSet->bZoneNames = false;
    m_pNetSet->bInstagib = false;
    m_pNetSet->fNameTagDrawDistance = 70.0f;
    m_pNetSet->bFriendlyFire = true;
    m_pNetSet->byteWorldTime_Hour = 12;
    m_pNetSet->byteWorldTime_Minute = 0;
    m_pNetSet->byteWeather = 1;
    m_pNetSet->fGravity = 0.008f;
    m_bNameTagStatus = true;

    m_dwLastConnectAttempt = GetTickCount();
    SetGameState(GAMESTATE_WAIT_CONNECT);
    m_bLanMode = false;

    pJavaWrapper->HideLoadingScreen();

    const char* sampVer = SAMP_VERSION;
    if(pSettings)
        sampVer = pSettings->Get().szVersion;
}
// 0.3.7
CNetGame::~CNetGame()
{
    // voice
    Network::OnRaknetDisconnect();

    m_pRakClient->Disconnect(0);
    UnregisterRPCs(m_pRakClient);
    UnregisterScriptRPCs(m_pRakClient);
    RakNetworkFactory::DestroyRakClientInterface(m_pRakClient);

    UninitializePools();

    if (m_pNetSet) {
        delete m_pNetSet;
        m_pNetSet = nullptr;
    }
}

void CNetGame::InitializePools()
{
    m_pPools = new NET_POOLS;
    m_pPools->pPlayerPool = new CPlayerPool();
    m_pPools->pVehiclePool = new CVehiclePool();
    m_pPools->pGangZonePool = new CGangZonePool();
    m_pPools->pPickupPool = new CPickupPool();
    m_pPools->pObjectPool = new CObjectPool();
    m_pPools->pTextLabelPool = new C3DTextLabelPool();
    m_pPools->pTextDrawPool = new CTextDrawPool();
    m_pPools->pActorPool = new CActorPool();
    m_pPools->pMenuPool = new CMenuPool();
    m_pPools->pPlayerBubblePool = new CPlayerBubblePool();
}

void CNetGame::UninitializePools()
{
    if (m_pPools->pPlayerPool) {
        delete m_pPools->pPlayerPool;
        m_pPools->pPlayerPool = nullptr;
    }

    if (m_pPools->pVehiclePool) {
        delete m_pPools->pVehiclePool;
        m_pPools->pVehiclePool = nullptr;
    }

    if (m_pPools->pGangZonePool) {
        delete m_pPools->pGangZonePool;
        m_pPools->pGangZonePool = nullptr;
    }

    if (m_pPools->pPickupPool) {
        delete m_pPools->pPickupPool;
        m_pPools->pPickupPool = nullptr;
    }

    if (m_pPools->pObjectPool) {
        delete m_pPools->pObjectPool;
        m_pPools->pObjectPool = nullptr;
    }

    if (m_pPools->pTextLabelPool) {
        delete m_pPools->pTextLabelPool;
        m_pPools->pTextLabelPool = nullptr;
    }

    if (m_pPools->pTextDrawPool) {
        delete m_pPools->pTextDrawPool;
        m_pPools->pTextDrawPool = nullptr;
    }

    if (m_pPools->pActorPool) {
        delete m_pPools->pActorPool;
        m_pPools->pActorPool = nullptr;
    }

    if (m_pPools->pMenuPool) {
        delete m_pPools->pMenuPool;
        m_pPools->pMenuPool = nullptr;
    }

    if (m_pPools->pPlayerBubblePool)
    {
        delete m_pPools->pPlayerBubblePool;
        m_pPools->pPlayerBubblePool = nullptr;
    }

    if (m_pPools) {
        delete m_pPools;
        m_pPools = nullptr;
    }

    if (m_pNetSet) {
        delete m_pNetSet;
        m_pNetSet = nullptr;
    }
}

void CNetGame::Process()
{
    static uint32_t time = GetTickCount();
    bool bProcess = false;
    if (GetTickCount() - time >= 1000 / 30)
    {
        if (m_iGameState == GAMESTATE_CONNECTING || m_iGameState == GAMESTATE_AWAIT_JOIN || m_iGameState == GAMESTATE_CONNECTED)
        {
            UpdateNetwork();
        }
        time = GetTickCount();
        bProcess = true;
    }
    CSpeedometr::update();
    if (m_pNetSet->byteHoldTime) {
        pGame->SetWorldTime(m_pNetSet->byteWorldTime_Hour, m_pNetSet->byteWorldTime_Minute);
    }

    //pGame->PreloadObjectsAnims();

    if (GetGameState() == GAMESTATE_CONNECTED) {
        ProcessPools();
    }
    else {
        ProcessLoadingScreen();
    }

    if (GetGameState() == GAMESTATE_WAIT_CONNECT) {
        ProcessConnecting();
    }
}

void CNetGame::UpdateNetwork()
{
    Packet *pkt = nullptr;
    unsigned char packetIdentifier;
    bool stopProcessing = false;

    while (!stopProcessing && (pkt = m_pRakClient->Receive()))
    {
        packetIdentifier = GetPacketID(pkt);

        switch (packetIdentifier) {
            case ID_AUTH_KEY:
                Packet_AuthKey(pkt);
                break;

            case ID_CONNECTION_ATTEMPT_FAILED:
                Packet_ConnectAttemptFailed(pkt);
                stopProcessing = true;
                break;

            case ID_NO_FREE_INCOMING_CONNECTIONS:
                Packet_NoFreeIncomingConnections(pkt);
                stopProcessing = true;
                break;

            case ID_DISCONNECTION_NOTIFICATION:
                Packet_DisconnectionNotification(pkt);
                stopProcessing = true;
                break;

            case ID_CONNECTION_LOST:
                Packet_ConnectionLost(pkt);
                stopProcessing = true;
                break;

            case ID_CONNECTION_REQUEST_ACCEPTED:
                Packet_ConnectionSucceeded(pkt);
                break;

            case ID_FAILED_INITIALIZE_ENCRIPTION:
                Packet_FailedInitializeEncription(pkt);
                break;

            case ID_CONNECTION_BANNED:
                Packet_ConnectionBanned(pkt);
                SetGameState(GAMESTATE_WAIT_CONNECT);
                stopProcessing = true;
                break;

            case ID_INVALID_PASSWORD:
                Packet_InvalidPassword(pkt);
                stopProcessing = true;
                break;

            case ID_VEHICLE_SYNC:
                Packet_VehicleSync(pkt);
                break;

            case ID_AIM_SYNC:
                Packet_AimSync(pkt);
                break;

            case ID_BULLET_SYNC:
                Packet_BulletSync(pkt);
                break;

            case ID_PLAYER_SYNC:
                Packet_PlayerSync(pkt);
                break;

            case ID_MARKERS_SYNC:
                Packet_MarkerSync(pkt);
                break;

            case ID_UNOCCUPIED_SYNC:
                Packet_UnoccupiedSync(pkt);
                break;

            case ID_TRAILER_SYNC:
                Packet_TrailerSync(pkt);
                break;

            case ID_PASSENGER_SYNC:
                Packet_PassengerSync(pkt);
                break;

            case Network::kRaknetPacketId:
                Network::OnRaknetReceive(*pkt);

            case 251:
                Packet_CustomRPC(pkt);
                break;
        }


        m_pRakClient->DeallocatePacket(pkt);
    }
}

void CNetGame::Packet_CustomRPC(Packet *p) {

    RakNet::BitStream bs((unsigned char *) p->data, p->length, false);
    bs.IgnoreBits(8); // skip packet id

    uint32_t rpcID;
    bs.Read(rpcID);

    switch (rpcID) {
        case 99: {
            FLog("RPC_CHECK_CASH");
            uint8_t bLen, bLen1;
            uint16_t bVersion;
            char szText[30];
            char szText1[30];

            memset(szText, 0, 30);
            memset(szText1, 0, 30);

            bs.Read(bLen);
            if (bLen >= sizeof(szText) - 1)
                return;

            bs.Read(&szText[0], bLen);

            bs.Read(bLen1);
            if (bLen1 >= sizeof(szText1) - 1)
                return;

            bs.Read(&szText1[0], bLen1);

            bs.Read(bVersion);

            RwTexture *pCashTexture = nullptr;
            pCashTexture = (RwTexture *) CUtil::LoadTextureFromDB(szText1, szText);

            int iVersion;
            if (pCashTexture) {
                iVersion = bVersion;
                RwTextureDestroy(pCashTexture);
            } else iVersion = 0;

            RakNet::BitStream bsParams;

            bsParams.Write((uint8_t) 251);
            bsParams.Write((uint32_t) 99);

            bsParams.Write(iVersion);

            m_pRakClient->Send(&bsParams, SYSTEM_PRIORITY, RELIABLE, 0);

//			bsParams.Write(iVersion);
//			m_pRakClient->RPC(&RPC_CustomHash, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
            break;
        }
        case RPC_SHOW_SPAWN_SELECTOR_WINDOW:
        {
            FLog("RPC_SHOW_SPAWN_SELECTOR_WINDOW");
            pJavaWrapper->ShowSpawn();
            break;
        }
        case RPC_SHOW_NOTIFICATION: {
            FLog("RPC_SHOW_NOTIFICATION");

            char szBuff[4096+1];
            char text[64*54];
            char actionBtn[64*54];
            char textBtn[64*54];

            //тип
            int type;
            bs.Read(type);

            //текст
            uint16_t lenText;
            bs.Read(lenText);

            if(lenText >= sizeof(text)) {
                FLog("Text length too large: %d", lenText);
                return;
            }

            memset(text, 0, sizeof(text));
            memset(szBuff, 0, sizeof(szBuff));

            bs.Read(szBuff, lenText);
            szBuff[lenText] = '\0';
            cp1251_to_utf8(text, szBuff);

            //время уведомления
            int duration;
            bs.Read(duration);

            //действие для кнопки
            uint16_t lenActionBtn;
            bs.Read(lenActionBtn);

            if(lenActionBtn >= sizeof(actionBtn)) {
                FLog("Action button text length too large: %d", lenActionBtn);
                return;
            }

            memset(actionBtn, 0, sizeof(actionBtn));
            memset(szBuff, 0, sizeof(szBuff));

            bs.Read(szBuff, lenActionBtn);
            szBuff[lenActionBtn] = '\0';
            cp1251_to_utf8(actionBtn, szBuff);

            //текст для кнопки
            uint16_t lenBtnText;
            bs.Read(lenBtnText);

            if(lenBtnText >= sizeof(textBtn)) {
                FLog("Button text length too large: %d", lenBtnText);
                return;
            }

            memset(textBtn, 0, sizeof(textBtn));
            memset(szBuff, 0, sizeof(szBuff));

            bs.Read(szBuff, lenBtnText);
            szBuff[lenBtnText] = '\0';
            cp1251_to_utf8(textBtn, szBuff);

            pJavaWrapper->ShowNotification(type, text, duration, actionBtn, textBtn);
            break;
        }
        default: {
            FLog("Unknown RPC ID: %d", rpcID);
            break;
        }
    }
}

// 0.3.7
void CNetGame::ShutdownForGameModeRestart()
{
    // voice
    SpeakerList::Hide();
    MicroIcon::Hide();
    Network::OnRaknetDisconnect();

    for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++)
    {
        CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(playerId);
        if (pRemotePlayer) {
            pRemotePlayer->SetTeam(NO_TEAM);
            pRemotePlayer->ResetAllSyncAttributes();
        }
    }

    GetPlayerPool()->GetLocalPlayer()->ResetAllSyncAttributes();
    GetPlayerPool()->GetLocalPlayer()->ToggleSpectating(false);
    GameResetStats();

    if (pAudioStream) { //add new
        pAudioStream->Stop(true);
    }

    SetGameState(GAMESTATE_RESTARTING);
    GetPlayerPool()->DeactivateAll();
    GetPlayerPool()->Process();
    ResetVehiclePool();
    ResetActorPool();
    ResetTextDrawPool();
    ResetGangZonePool();
    Reset3DTextLabelPool();
    ResetPickupPool();
    ResetObjectPool();
    ResetMenuPool();

    m_pNetSet->bDisableInteriorEnterExits = false;
    m_pNetSet->fNameTagDrawDistance = 70.0f;
    m_pNetSet->byteWorldTime_Hour = 12;
    m_pNetSet->byteWorldTime_Minute = 0;
    m_pNetSet->byteWeather = 1;
    m_pNetSet->byteHoldTime = 1;
    m_pNetSet->bNameTagLOS = true;
    m_pNetSet->bUseCJWalk = false;
    pGame->ToggleCJWalk(false);
    m_pNetSet->fGravity = 0.008f;
    m_pNetSet->iDeathDropMoney = 0;

    pGame->m_bCheckpointsEnabled = false;
    pGame->m_bRaceCheckpointsEnabled = false;

    pGame->FindPlayerPed()->m_pPed->SetInterior(0, true);
    pGame->ResetLocalMoney();
    pGame->FindPlayerPed()->SetDead();
    pGame->FindPlayerPed()->SetArmour(0.0f);
    pGame->EnableZoneNames(false);
    m_pNetSet->bZoneNames = false;
    GameResetRadarColors();
    pGame->SetGravity(m_pNetSet->fGravity);
    pGame->SetWantedLevel(0);
    pGame->EnableClock(false);

    // voice
    SpeakerList::Hide();
    MicroIcon::Hide();
    Network::OnRaknetDisconnect();

    if (pUI) pUI->chat()->addInfoMessage("The server is restarting..");
}

int iVehiclePoolProcessFlag = 0;
void CNetGame::ProcessPools()
{
    if (GetPlayerPool()) {
        GetPlayerPool()->Process();
    }

    if(GetVehiclePool()) {
        GetVehiclePool()->Process();
    }

    if (GetPickupPool()) {
        GetPickupPool()->Process();
    }
}
// 0.3.7
void CNetGame::ProcessLoadingScreen()
{
    CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
    if (pPlayerPed->IsInVehicle()) {
        pPlayerPed->RemoveFromVehicleAndPutAt(1093.4, -2036.5, 82.710602);
    }
    else {
        pPlayerPed->m_pPed->SetPosn(1133.0504, -2038.4034, 69.099998);
    }
    CCamera::SetPosition(1093.0, -2036.0, 90.0, 0.0, 0.0, 0.0);
    CCamera::LookAtPoint(384.0, -1557.0, 20.0, 2);
    pGame->SetWorldWeather(1);
    pGame->DisplayHUD(false);
}
// 0.3.7
void CNetGame::ProcessConnecting()
{
    if (GetTickCount() - m_dwLastConnectAttempt > 100/*3000*/)
    {
        if (pUI) pUI->chat()->addDebugMessage("Подключение к серверу...");

        m_pRakClient->Connect(m_szHostOrIp, m_iPort, 0, 0, 2);

        // voice fix voice not connect when restart
        Network::OnRaknetConnect(m_szHostOrIp, m_iPort);

        m_dwLastConnectAttempt = GetTickCount();
        SetGameState(GAMESTATE_CONNECTING);
    }
}
// 0.3.7
void gen_auth_key(char buf[260], char* auth_in);
void CNetGame::Packet_AuthKey(Packet *pkt)
{
    uint8_t byteAuthLen;
    char szAuth[260], szAuthKey[269];

    RakNet::BitStream bsAuth((unsigned char*)pkt->data, pkt->length, false);

    if (GetGameState() < GAMESTATE_WAIT_CONNECT || GetGameState() > GAMESTATE_AWAIT_JOIN) return;

    bsAuth.IgnoreBits(8);
    bsAuth.Read(byteAuthLen);
    bsAuth.Read(szAuth, byteAuthLen);
    szAuth[byteAuthLen] = '\0';

    gen_auth_key(szAuthKey, szAuth);
    uint8_t byteAuthKeyLen = strlen(szAuthKey);

    RakNet::BitStream bsKey;
    bsKey.Write((uint8_t)ID_AUTH_KEY);
    bsKey.Write(byteAuthKeyLen);
    bsKey.Write(szAuthKey, byteAuthKeyLen);
    m_pRakClient->Send(&bsKey, SYSTEM_PRIORITY, RELIABLE, 0);
}
// 0.3.7
void CNetGame::Packet_ConnectAttemptFailed(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Сервер не ответил. Повторяем попытку...");
    if (pAudioStream) { //add new
        pAudioStream->Stop(true);
    }
    SpeakerList::Hide(); //add new
    MicroIcon::Hide();
    Network::OnRaknetDisconnect();
    SetGameState(GAMESTATE_WAIT_CONNECT);

    //SpeakerList::Hide();
    //MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_NoFreeIncomingConnections(Packet *pkt)
{
    if(pUI) pUI->chat()->addDebugMessage("Сервер перегружен. Повторяем попытку...");
    SpeakerList::Hide(); //addnew
    MicroIcon::Hide();
    Network::OnRaknetDisconnect();
    SetGameState(GAMESTATE_WAIT_CONNECT);

    //SpeakerList::Hide();
    //MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_DisconnectionNotification(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Сервер прервал соединение.");
    if (pAudioStream) {
        pAudioStream->Stop(true);
    }
    m_pRakClient->Disconnect(2000);

    SpeakerList::Hide();
    MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_ConnectionSucceeded(Packet *pkt)
{
    RakNet::BitStream bsSuccAuth(pkt->data, pkt->length, true);
    PLAYERID MyPlayerID;
    unsigned int uiChallenge;
    int iVersion = 4057;
    uint8_t byteMod = 1;

    bsSuccAuth.IgnoreBits(8);	// packetId
    bsSuccAuth.IgnoreBits(32);	// binaryAddress
    bsSuccAuth.IgnoreBits(16);	// port

    bsSuccAuth.Read(MyPlayerID);
    bsSuccAuth.Read(uiChallenge);
    uiChallenge ^= iVersion;

    if (pUI) pUI->chat()->addDebugMessage("Соединено. Подготовка к игре...");

    SetGameState(GAMESTATE_AWAIT_JOIN);

    uint8_t byteNameLen = strlen(GetPlayerPool()->GetLocalPlayerName());
    uint8_t byteAuthBSLen = strlen(AUTH_BS);
    uint8_t byteClientVerLen = strlen(SAMP_VERSION);
    if(pSettings)
        byteClientVerLen = strlen(pSettings->Get().szVersion);


    RakNet::BitStream bsSend;
    bsSend.Write(iVersion);
    bsSend.Write(byteMod);
    bsSend.Write(byteNameLen);
    bsSend.Write(GetPlayerPool()->GetLocalPlayerName(), byteNameLen);
    bsSend.Write(uiChallenge);
    bsSend.Write(byteAuthBSLen);
    bsSend.Write(AUTH_BS, byteAuthBSLen);
    bsSend.Write(byteClientVerLen);
    //bsSend.Write(SAMP_VERSION, byteClientVerLen);
    if(pSettings)
        bsSend.Write(pSettings->Get().szVersion, byteClientVerLen);
    else
        bsSend.Write(SAMP_VERSION, byteClientVerLen);

    Network::OnRaknetRpc(RPC_ClientJoin, bsSend);

    m_pRakClient->RPC(&RPC_ClientJoin, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);

    // voice
    SpeakerList::Hide();
    MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_FailedInitializeEncription(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Не удалось инициализировать шифрование.");
}
// 0.3.7
void CNetGame::Packet_ConnectionBanned(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Вам запрещено заходить на этот сервер.");
}
// 0.3.7
void CNetGame::Packet_InvalidPassword(Packet *pkt)
{
    if (pUI) pUI->chat()->addDebugMessage("Неверный пароль сервера.");
    m_pRakClient->Disconnect(0);
}
// 0.3.7
void CNetGame::Packet_ConnectionLost(Packet *pkt)
{
    if (m_pRakClient) {
        m_pRakClient->Disconnect(0);
    }

    if (pUI) pUI->chat()->addDebugMessage("Потеряно соединение с сервером. Повторное подключение...");
    ShutdownForGameModeRestart();

    CPlayerPool *pPlayerPool = GetPlayerPool();
    if (pPlayerPool) {
        for (PLAYERID playerId = 0; playerId < MAX_PLAYERS; playerId++) {
            if (pPlayerPool->GetSlotState(playerId) == true) {
                pPlayerPool->Delete(playerId, 0);
            }
        }
    }

    if (pAudioStream) {
        pAudioStream->Stop(true);
    }

    SetGameState(GAMESTATE_WAIT_CONNECT);

    SpeakerList::Hide();
    MicroIcon::Hide();
}
// 0.3.7
void CNetGame::Packet_PlayerSync(Packet *pkt)
{
    RakNet::BitStream bsData(pkt->data, pkt->length, false);
    ONFOOT_SYNC_DATA ofSync;
    uint32_t dwTime = 0;
    uint8_t bytePacketId;
    PLAYERID playerId;

    bool bHasLR, bHasUD;

    if (GetGameState() != GAMESTATE_CONNECTED) return;

    memset(&ofSync, 0, sizeof(ONFOOT_SYNC_DATA));

    bsData.Read(bytePacketId);
    bsData.Read(playerId);

    bsData.Read(bHasLR);
    if (bHasLR) {
        bsData.Read(ofSync.lrAnalog);
    }

    bsData.Read(bHasUD);
    if (bHasUD) {
        bsData.Read(ofSync.udAnalog);
    }

    bsData.Read(ofSync.wKeys);
    bsData.Read((char*)&ofSync.vecPos, sizeof(CVector));
    float w, x, y, z;
    bsData.ReadNormQuat(w, x, y, z);
    ofSync.quat.Set(x, y, z, w);

    uint8_t byteHealthArmour;
    uint8_t byteArmTemp = 0, byteHlTemp = 0;

    bsData.Read(byteHealthArmour);
    byteArmTemp = (byteHealthArmour & 0x0F);
    byteHlTemp = (byteHealthArmour >> 4);

    if (byteArmTemp == 0xF) ofSync.byteArmour = 100;
    else if (byteArmTemp == 0) ofSync.byteArmour = 0;
    else ofSync.byteArmour = byteArmTemp * 7;

    if (byteHlTemp == 0xF) ofSync.byteHealth = 100;
    else if (byteHlTemp == 0) ofSync.byteHealth = 0;
    else ofSync.byteHealth = byteHlTemp * 7;

    uint8_t byteCurrentWeapon = 0;
    bsData.Read(byteCurrentWeapon);
    ofSync.byteCurrentWeapon ^= (byteCurrentWeapon ^ ofSync.byteCurrentWeapon) & 0x3F;

    bsData.Read(ofSync.byteSpecialAction);
    bsData.ReadVector(ofSync.vecMoveSpeed.x, ofSync.vecMoveSpeed.y, ofSync.vecMoveSpeed.z);

    bool bHasVehicleSurfingInfo;
    bsData.Read(bHasVehicleSurfingInfo);
    if (bHasVehicleSurfingInfo)
    {
        bsData.Read(ofSync.wSurfID);
        bsData.Read(ofSync.vecSurfOffsets.x);
        bsData.Read(ofSync.vecSurfOffsets.y);
        bsData.Read(ofSync.vecSurfOffsets.z);
    }
    else
        ofSync.wSurfID = INVALID_VEHICLE_ID;

    bool bHasAnimation;
    bsData.Read(bHasAnimation);
    if (bHasAnimation) {
        bsData.Read(ofSync.dwAnimation);
    }
    else {
        ofSync.dwAnimation = 0x80000000;
    }

    CRemotePlayer *pRemotePlayer = GetPlayerPool()->GetAt(playerId);
    if (pRemotePlayer) {
        pRemotePlayer->StoreOnFootFullSyncData(&ofSync, 0);
    }
}
// 0.3.7
void CNetGame::Packet_VehicleSync(Packet* pkt)
{
    RakNet::BitStream bsData(pkt->data, pkt->length, false);
    uint32_t dwTime = 0;
    uint8_t bytePacketId;
    INCAR_SYNC_DATA icSync;
    PLAYERID PlayerID;

    if (GetGameState() != GAMESTATE_CONNECTED) return;

    memset(&icSync, 0, sizeof(INCAR_SYNC_DATA));

    bsData.Read(bytePacketId);
    bsData.Read(PlayerID);
    bsData.Read(icSync.VehicleID);
    bsData.Read(icSync.lrAnalog);
    bsData.Read(icSync.udAnalog);
    bsData.Read(icSync.wKeys);

    float w, x, y, z;
    bsData.ReadNormQuat(w, x, y, z);
    icSync.quat.Set(x, y, z, w);

    bsData.Read((char*)& icSync.vecPos, sizeof(CVector));
    bsData.ReadVector(icSync.vecMoveSpeed.x, icSync.vecMoveSpeed.y, icSync.vecMoveSpeed.z);

    // car health
    uint16_t wTempVehicleHealth;
    bsData.Read(wTempVehicleHealth);
    icSync.fCarHealth = (float)wTempVehicleHealth;

    // health/armour
    uint8_t byteHealthArmour;
    uint8_t byteArmTemp = 0, byteHlTemp = 0;

    bsData.Read(byteHealthArmour);
    byteArmTemp = (byteHealthArmour & 0x0F);
    byteHlTemp = (byteHealthArmour >> 4);

    if (byteArmTemp == 0xF) icSync.bytePlayerArmour = 100;
    else if (byteArmTemp == 0) icSync.bytePlayerArmour = 0;
    else icSync.bytePlayerArmour = byteArmTemp * 7;

    if (byteHlTemp == 0xF) icSync.bytePlayerHealth = 100;
    else if (byteHlTemp == 0) icSync.bytePlayerHealth = 0;
    else icSync.bytePlayerHealth = byteHlTemp * 7;

    // current weapon
    uint8_t byteTempWeapon;
    bsData.Read(byteTempWeapon);
    icSync.byteCurrentWeapon ^= (byteTempWeapon ^ icSync.byteCurrentWeapon) & 0x3F;

    bool bCheck;

    // siren
    bsData.Read(bCheck);
    if (bCheck) icSync.byteSirenOn = 1;
    // landinggear
    bsData.Read(bCheck);
    if (bCheck) icSync.byteLandingGearState = 1;
    // train speed
    bsData.Read(bCheck);
    if (bCheck) bsData.Read(icSync.fTrainSpeed);
    // triler id
    bsData.Read(bCheck);
    if (bCheck) bsData.Read(icSync.TrailerID);

    CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(PlayerID);
    if (pRemotePlayer) {
        pRemotePlayer->StoreInCarFullSyncData(&icSync, 0);
    }
}
// 0.3.7
void CNetGame::Packet_AimSync(Packet* pkt)
{
    RakNet::BitStream bsData(pkt->data, pkt->length, false);

    uint8_t bytePacketId;
    PLAYERID PlayerId;
    AIM_SYNC_DATA aimSync;
    bsData.Read(bytePacketId);
    bsData.Read(PlayerId);
    bsData.Read((char*)& aimSync, sizeof(AIM_SYNC_DATA));

    CRemotePlayer* pPlayer = m_pPools->pPlayerPool->GetAt(PlayerId);
    if (pPlayer)
        pPlayer->StoreAimFullSyncData(&aimSync);
}
// 0.3.7
void CNetGame::Packet_BulletSync(Packet* pkt)
{
    RakNet::BitStream bsData(pkt->data, pkt->length, false);

    if (GetGameState() != GAMESTATE_CONNECTED) return;

    BULLET_SYNC_DATA btSync;
    uint8_t bytePacketId = 0;
    PLAYERID PlayerID = 0;

    bsData.Read(bytePacketId);
    bsData.Read(PlayerID);
    bsData.Read((char*)&btSync, sizeof(BULLET_SYNC_DATA));

    CPlayerPool* pPlayerPool = GetPlayerPool();
    CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(PlayerID);
    if (pRemotePlayer)
        pRemotePlayer->StoreBulletFullSyncData(&btSync);

}
// 0.3.7
void CNetGame::Packet_PassengerSync(Packet* pkt)
{
    RakNet::BitStream bsData(pkt->data, pkt->length, false);

    if (GetGameState() == GAMESTATE_CONNECTED)
    {
        uint8_t bytePacketId;
        PLAYERID PlayerID;
        PASSENGER_SYNC_DATA psSync;

        bsData.Read(bytePacketId);
        bsData.Read(PlayerID);
        bsData.Read((char*)& psSync, sizeof(PASSENGER_SYNC_DATA));

        CRemotePlayer* pRemotePlayer = GetPlayerPool()->GetAt(PlayerID);
        if (pRemotePlayer) {
            pRemotePlayer->StorePassengerFullSyncData(&psSync);
        }
    }
}

// 0.3.7
void CNetGame::Packet_MarkerSync(Packet *pkt)
{
    if(m_iGameState != GAMESTATE_CONNECTED) return;

    uint8_t bytePacketId;
    int	iNumberOfPlayers;
    PLAYERID playerId;
    bool bIsPlayerActive;
    short sPos[3];

    RakNet::BitStream bsMarkersSync((unsigned char *)pkt->data, pkt->length, false);
    bsMarkersSync.Read(bytePacketId);
    bsMarkersSync.Read(iNumberOfPlayers);
    if(iNumberOfPlayers)
    {
        for(int i = 0; i < iNumberOfPlayers; i++)
        {
            bsMarkersSync.Read(playerId);
            bsMarkersSync.ReadCompressed(bIsPlayerActive);

            if(bIsPlayerActive)
            {
                bsMarkersSync.Read(sPos[0]);
                bsMarkersSync.Read(sPos[1]);
                bsMarkersSync.Read(sPos[2]);
            }

            if(playerId >= 0 && playerId < MAX_PLAYERS && GetPlayerPool()->GetSlotState(playerId))
            {
                CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
                if(pPlayer)
                {
                    if(bIsPlayerActive)
                        pPlayer->ShowGlobalMarker(sPos[0], sPos[1], sPos[2]);
                    else
                        pPlayer->HideGlobalMarker();
                }
            }
        }
    }
}

/*
void CNetGame::Packet_VoiceChannelOpenReply(Packet* pkt)
{
	FLog("Packet_VoiceChannelOpenReply");

	if (!pVoice || !GetGameState() == GAMESTATE_CONNECTED) return;

	GetPlayerPool()->GetLocalPlayer()->VoiceChannelAccept();
}*/

/*
void CNetGame::Packet_VoiceData(Packet* pkt)
{
	if (!pVoice || !GetGameState() == GAMESTATE_CONNECTED) return;

	PLAYERID playerId;
	int size;
	unsigned char data[MAX_VOICE_PACKET_SIZE];
	RakNet::BitStream bsData(pkt->data, pkt->length, false);
	bsData.IgnoreBits(8);
	bsData.Read(playerId);
	bsData.Read(size);
	bsData.Read((char*)data, size);

	if (GetPlayerPool()->GetSlotState(playerId)) {
		pVoice->Push(playerId, data, size);
	}
}*/

// 0.3.7
void CNetGame::UpdatePlayerScoresAndPings()
{
    static uint32_t dwLastUpdateTick = 0;

    if (GetTickCount() - dwLastUpdateTick > 3000) {
        dwLastUpdateTick = GetTickCount();

        RakNet::BitStream bsSend;
        m_pRakClient->RPC(&RPC_UpdateScoresPingsIPs, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
    }
}

void CNetGame::SendDialogResponse(uint16_t wDialogID, uint8_t byteButtonID, uint16_t wListBoxItem, const char* szInput)
{
    if (GetGameState() != GAMESTATE_CONNECTED) return;

    uint8_t length = strlen(szInput);

    RakNet::BitStream bsSend;
    bsSend.Write(wDialogID);
    bsSend.Write(byteButtonID);
    bsSend.Write(wListBoxItem);
    bsSend.Write(length);
    bsSend.Write(szInput, length);
    m_pRakClient->RPC(&RPC_DialogResponse, &bsSend, HIGH_PRIORITY, RELIABLE_ORDERED, 0, false, UNASSIGNED_NETWORK_ID, nullptr);
}

void CNetGame::SendChatMessage(const char* szMsg)
{
    if (GetGameState() != GAMESTATE_CONNECTED) return;

    RakNet::BitStream bsSend;
    uint8_t byteTextLen = strlen(szMsg);

    bsSend.Write(byteTextLen);
    bsSend.Write(szMsg, byteTextLen);

    m_pRakClient->RPC(&RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}

void CNetGame::SendChatCommand(const char* szCommand)
{
    if (GetGameState() != GAMESTATE_CONNECTED) return;

    RakNet::BitStream bsParams;
    int iStrlen = strlen(szCommand);

    bsParams.Write(iStrlen);
    bsParams.Write(szCommand, iStrlen);
    m_pRakClient->RPC(&RPC_ServerCommand, &bsParams, HIGH_PRIORITY, RELIABLE, 0, false, UNASSIGNED_NETWORK_ID, NULL);
}
// 0.3.7
void CNetGame::SetMapIcon(uint8_t byteIconID, float fPosX, float fPosY, float fPosZ, uint8_t byteType, uint32_t dwColor, uint8_t byteStyle)
{
    if (m_dwMapIcon[byteIconID] != 0) {
        DisableMapIcon(byteIconID);
    }

    m_dwMapIcon[byteIconID] = pGame->CreateRadarMarkerIcon(byteType, fPosX, fPosY, fPosZ, dwColor, byteStyle);
}
// 0.3.7
void CNetGame::DisableMapIcon(uint8_t byteIconID)
{
    ScriptCommand(&disable_marker, m_dwMapIcon[byteIconID]);
    m_dwMapIcon[byteIconID] = 0;
}
// 0.3.7
void CNetGame::ResetVehiclePool()
{
    if (m_pPools->pVehiclePool) {
        delete m_pPools->pVehiclePool;
    }

    m_pPools->pVehiclePool = new CVehiclePool();
}
// 0.3.7
void CNetGame::ResetActorPool()
{
    if (m_pPools->pActorPool) {
        delete m_pPools->pActorPool;
    }

    m_pPools->pActorPool = new CActorPool();
}
// 0.3.7
void CNetGame::ResetTextDrawPool()
{
    if (m_pPools->pTextDrawPool) {
        delete m_pPools->pTextDrawPool;
    }

    m_pPools->pTextDrawPool = new CTextDrawPool();
}
// 0.3.7
void CNetGame::ResetGangZonePool()
{
    if (m_pPools->pGangZonePool) {
        delete m_pPools->pGangZonePool;
    }

    m_pPools->pGangZonePool = new CGangZonePool();
}
// 0.3.7
void CNetGame::Reset3DTextLabelPool()
{
    if (m_pPools->pTextLabelPool) {
        delete m_pPools->pTextLabelPool;
    }

    m_pPools->pTextLabelPool = new C3DTextLabelPool();
}
// 0.3.7
void CNetGame::ResetMapIcons()
{
    for (int i = 0; i < MAX_MAP_ICONS; i++)
    {
        if (m_dwMapIcon[i]) {
            ScriptCommand(&disable_marker, m_dwMapIcon[i]);
            m_dwMapIcon[i] = 0;
        }
    }
}
// 0.3.7
void CNetGame::ResetPickupPool()
{
    if (m_pPools->pPickupPool) {
        delete m_pPools->pPickupPool;
    }

    m_pPools->pPickupPool = new CPickupPool();
}
// 0.3.7
void CNetGame::ResetObjectPool()
{
    if (m_pPools->pObjectPool) {
        delete m_pPools->pObjectPool;
    }

    m_pPools->pObjectPool = new CObjectPool();
}
// 0.3.7
void CNetGame::ResetMenuPool()
{
    if (m_pPools->pMenuPool) {
        delete m_pPools->pMenuPool;
    }

    m_pPools->pMenuPool = new CMenuPool();
}
// 0.3.7
void CNetGame::InitGameLogic()
{
    if (m_pNetSet->bManualVehicleEngineAndLight) {
        //InstallVehicleEngineLightPatches();
    }

    m_pNetSet->fWorldBounds[0] = 20000.0f;
    m_pNetSet->fWorldBounds[1] = -20000.0f;
    m_pNetSet->fWorldBounds[2] = 20000.0f;
    m_pNetSet->fWorldBounds[3] = -20000.0f;
}

void CNetGame::SendPrevClass()
{
    CPlayerPool* pPlayerPool = GetPlayerPool();
    if (pPlayerPool)
    {
        pPlayerPool->GetLocalPlayer()->SendPrevClass();
    }
}

void CNetGame::SendSpawn()
{
    CPlayerPool* pPlayerPool = GetPlayerPool();
    if (pPlayerPool)
    {
        pPlayerPool->GetLocalPlayer()->SendSpawn();
    }
}

void CNetGame::SendNextClass()
{
    CPlayerPool* pPlayerPool = GetPlayerPool();
    if (pPlayerPool)
    {
        pPlayerPool->GetLocalPlayer()->SendNextClass();
    }
}

void CNetGame::Packet_TrailerSync(Packet *pkt)
{
    if(GetGameState() != GAMESTATE_CONNECTED) return;

    uint8_t bytePacketId;
    PLAYERID playerId;

    TRAILER_SYNC_DATA trSync;
    memset(&trSync, 0, sizeof(TRAILER_SYNC_DATA));

    RakNet::BitStream bsTrailerSync((unsigned char *)pkt->data, pkt->length, false);
    bsTrailerSync.Read(bytePacketId);
    bsTrailerSync.Read(playerId);
    bsTrailerSync.Read((char*)&trSync, sizeof(TRAILER_SYNC_DATA));

    if(GetPlayerPool())
    {
        CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
        if(pPlayer)
            pPlayer->StoreTrailerFullSyncData(&trSync);
    }
}

void CNetGame::Packet_UnoccupiedSync(Packet *pkt)
{
    if(m_iGameState != GAMESTATE_CONNECTED) return;

    uint8_t bytePacketId;
    PLAYERID playerId;

    UNOCCUPIED_SYNC_DATA unocSync;
    memset(&unocSync, 0, sizeof(UNOCCUPIED_SYNC_DATA));

    RakNet::BitStream bsUnocSync((unsigned char *)pkt->data, pkt->length, false);
    bsUnocSync.Read(bytePacketId);
    bsUnocSync.Read(playerId);
    bsUnocSync.Read((char*)&unocSync,sizeof(UNOCCUPIED_SYNC_DATA));

    if(GetPlayerPool())
    {
        CRemotePlayer *pPlayer = GetPlayerPool()->GetAt(playerId);
        if(pPlayer)
            pPlayer->StoreUnoccupiedSyncData(&unocSync);
    }
}
