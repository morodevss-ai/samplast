#include "Ped.h"
#include "patch.h"
#include "game/General.h"
#include "game/Streaming.h"
#include "java/Hud.h"
#include "util.h"

void CPed::GetBonePosition(RwV3d *posn, uint32 boneTag, bool bCalledFromCamera) {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x004A4B0C + 1 : 0x59AEE4), this, posn, boneTag, bCalledFromCamera);
}

bool CPed::IsEnteringCar() {
    if ( GetTaskManager().CTaskManager::FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_DRIVER) )
        return true;

    return GetTaskManager().CTaskManager::FindActiveTaskByType(TASK_COMPLEX_ENTER_CAR_AS_PASSENGER) != nullptr;
}

bool CPed::IsExitingVehicle() {
    if ( GetTaskManager().CTaskManager::FindActiveTaskByType(TASK_COMPLEX_LEAVE_CAR) )
        return true;

    return false;
}

CPed::CPed(ePedType pedType) : CPhysical(), m_pedIK(reinterpret_cast<CPedGTA *>(this)) {
    m_vecAnimMovingShiftLocal = CVector2D();

    m_fHealth = 100.0f;
    m_fMaxHealth = 100.0f;
    m_fArmour = 0.0f;

    m_nPedType = pedType;
    m_nType = ENTITY_TYPE_PED;

    physicalFlags.bCanBeCollidedWith = true;
    physicalFlags.bDisableTurnForce = true;

    m_nCreatedBy = PED_GAME;
    pVehicle = nullptr;
    m_nAntiSpazTimer = 0;
    m_nUnconsciousTimer = 0;
    m_nAttackTimer = 0;
    m_nLookTime = 0;
    m_nDeathTimeMS = 0;

    m_vecAnimMovingShift = CVector2D();

    m_nPedState = PEDSTATE_IDLE;
    m_nMoveState = PEDMOVE_STILL;
    m_fCurrentRotation = 0.0f;
    m_fHeadingChangeRate = 15.0f;
    m_fMoveAnim = 0.1f;
    m_fAimingRotation = 0.0f;
    m_pFire = nullptr;
    m_fireDmgMult = 1.0f;
    m_pLookTarget = nullptr;
    m_fLookDirection = 0.0f;
    m_fMass = 70.0f;
    m_fTurnMass = 100.0f;
    m_fAirResistance = 1.f / 175.f;
    m_fElasticity = 0.05f;
    bHasACamera = CGeneral::GetRandomNumber() % 4 != 0;

    m_nSavedWeapon = WEAPON_UNIDENTIFIED;
    m_nDelayedWeapon = WEAPON_UNIDENTIFIED;
    m_nActiveWeaponSlot = 0;

    for (auto& weapon : m_aWeapons ) {
        weapon.dwType = WEAPON_UNARMED;
        weapon.dwState = WEAPONSTATE_READY;
        weapon.dwAmmoInClip = 0;
        weapon.dwAmmo = 0;
    }

    m_nWeaponSkill = eWeaponSkill::STD;
    m_nFightingStyle = STYLE_STANDARD;
    m_nAllowedAttackMoves = 0;

    m_nWeaponAccuracy = 60;
    m_pLastEntityDamage = nullptr;
    m_pAttachedTo = nullptr;
    m_nWeaponModelId = -1;
    field_72F = 0;
    m_VehDeadInFrontOf = nullptr;

    m_pWeaponClump = nullptr;
    m_pWeaponFlashFrame = nullptr;
    m_pGogglesClump = nullptr;
    m_pbGogglesEffect = nullptr;

    m_nWeaponGunflashAlphaMP1 = 0;
    m_nWeaponGunFlashAlphaProgMP1 = 0;
    m_nWeaponGunflashAlphaMP2 = 0;
    m_nWeaponGunFlashAlphaProgMP2 = 0;

    m_pCoverPoint = nullptr;
    m_pPlayerData = nullptr;

    m_fRemovalDistMultiplier = 1.0f;
    m_nSpecialModelIndex = -1;
}

CPed::~CPed() {
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x49F6A4 + 1 : 0x59541C), this);
}

void CPed::GiveWeapon(int iWeaponID, int iAmmo)
{
    int iModelID = GameGetWeaponModelIDFromWeaponID(iWeaponID);
    FLog("GiveWeapon, iModelID %d, iWeaponID %d", iModelID, iWeaponID);

    if (iModelID == -1 || iModelID == 350 || iModelID == 365) return;

    if (!CStreaming::TryLoadModel(iModelID))
        return;

    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x0049F588 + 1 : 0x59525C), this, iWeaponID, iAmmo);
    CHook::CallFunction<void>(g_libGTASA + (VER_x32 ? 0x004A521C + 1 : 0x59B86C), this, iWeaponID);

    CHud::ForceUpdateAmmo();
}

void CPed::RemoveFromVehicle()
{
    if(!IsInVehicle())
        return;

    m_pIntelligence->m_TaskMgr.FlushImmediately();

    //auto pos = pVehicle->GetPosition();
    //float ang = pVehicle->GetHeading();

    //pos.x += (1.0f * sin(ang + 4.71239f));
    //pos.y += (1.0f * sin(-ang + 4.71239f));

    //SetPosn(pos);
}

void CPed::RemoveFromVehicleAndPutAt(const CVector& pos)
{
    if (!this) return;
    if(!pVehicle) return;

    m_pIntelligence->m_TaskMgr.FlushImmediately();

    SetPosn(pos);

    UpdateRW();
    UpdateRwFrame();
}