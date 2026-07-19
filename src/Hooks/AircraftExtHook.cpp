#include <exception>
#include <Windows.h>

#include <GeneralStructures.h>
#include <TechnoClass.h>
#include <FootClass.h>
#include <AircraftClass.h>
#include <MapClass.h>

#include <Extension.h>
#include <Utilities/Macro.h>

#include <Extension/TechnoExt.h>
#include <Extension/TechnoTypeExt.h>
#include <Extension/WarheadTypeExt.h>

#include <Ext/Common/CommonStatus.h>
#include <Ext/TechnoType/TechnoStatus.h>
#include <Ext/TechnoType/AircraftAttitude.h>
#include <Ext/TechnoType/AircraftDive.h>
#include <Ext/TechnoType/AircraftGuard.h>

DEFINE_HOOK(0x639DD8, PlanningManager_AllowAircraftsWaypoint, 0x5)
{
	GET(TechnoClass*, pTechno, ESI);
	switch (pTechno->WhatAmI())
	{
	case AbstractType::Infantry:
	case AbstractType::Unit:
		return 0x639DDD;
	case AbstractType::Aircraft:
		if (!pTechno->GetTechnoType()->Spawned)
		{
			return 0x639DDD;
		}
	}
	return 0x639E03;
}

#pragma region DrawShadow
// Phobos took over the entire shadow rendering process, so skip Phobos's Hook here
DEFINE_HOOK(0x73C47A, UnitClass_DrawAsVXL_Shadow_SkipPhobos, 0x5)
{
	if (AudioVisual::Data()->AllowTakeoverPhobosShadowMaker)
	{
		GET(UnitClass*, pThis, EBP);
		int height = pThis->GetHeight();
		R->EAX(height);
		ILocomotion* pLoco = pThis->Locomotor.get();
		if (pThis->CloakState != CloakState::Uncloaked || pThis->Type->NoShadow || !pLoco->Is_To_Have_Shadow())
		{
			// 彻底跳过阴影绘制过程，不论是Phobos还是原版
			return 0x73C5C9;
		}
		// 只跳过Phobos的Hook, 继续下一步
		return 0x73C485;
	}
	return 0;
}

// Phobos took over the entire shadow rendering process, so skip Phobos's Hook here
DEFINE_HOOK(0x4147F9, AircraftClass_Draw_Shadow_SkipPhobos, 0x6)
{
	if (AudioVisual::Data()->AllowTakeoverPhobosShadowMaker)
	{
		GET(AircraftClass*, pThis, EBP);
		ILocomotion* pLoco = pThis->Locomotor.get();
		R->EAX(pLoco);
		if (pThis->Type->NoShadow || pThis->CloakState != CloakState::Uncloaked || pThis->IsSinking || !pLoco->Is_To_Have_Shadow())
		{
			// 彻底跳过阴影绘制过程，不论是Phobos还是原版
			return 0x4148A5;
		}
		// 只跳过Phobos的Hook, 继续下一步
		return 0x4147FF;
	}
	return 0;
}

// Phobos skip the entire shadow process, there are no phobos if this hook action
DEFINE_HOOK(0x707323, TechnoClass_DrawShadow_SkipAircraftScale, 0x5)
{
	// There are no have Phobos b39
	return 0x707331;
}

DEFINE_HOOK_AGAIN(0x73C4F8, TechnoClass_DrawShadow, 0x7) // InAir
DEFINE_HOOK_AGAIN(0x73C58E, TechnoClass_DrawShadow, 0x7) // OnGround
DEFINE_HOOK(0x414876, TechnoClass_DrawShadow, 0x7) // Aircraft
{
	GET(TechnoClass*, pTechno, EBP);
	GET(Matrix3D*, pMatrix, EAX);
	TechnoTypeClass* pType = pTechno->GetTechnoType();
	if (pType->ConsideredAircraft || pTechno->WhatAmI() == AbstractType::Aircraft)
	{
		// 修复子机导弹的影子位置
		if (pType->MissileSpawn && !pType->NoShadow)
		{
			CoordStruct location = pTechno->GetCoords();
			CellClass* pCell = MapClass::Instance->TryGetCellAt(location);
			location.Z = pCell->GetCoordsWithBridge().Z;
			Point2D pos = ToClientPos(location);
			R->Stack(0x30, pos);
		}
		// 缩放影子
		pMatrix->Scale(AudioVisual::Data()->VoxelShadowScaleInAir);
		if (pType->MissileSpawn || pTechno->IsInAir())
		{
			// 调整倾斜时影子的纵向比例
			FootClass* pFoot = abstract_cast<FootClass*, true>(pTechno);
			// 从Matrix中读取的角度不可用
			float x = 0; // 倾转轴
			float y = 0; // 俯仰轴
			// 火箭的俯仰角度，由RocketLoco记录
			if (RocketLocomotionClass* rLoco = dynamic_cast<RocketLocomotionClass*>(pFoot->Locomotor.get()))
			{
				x = pTechno->AngleRotatedSideways;
				y = rLoco->CurrentPitch;
			}
			else if (FlyLocomotionClass* fLoco = dynamic_cast<FlyLocomotionClass*>(pFoot->Locomotor.get()))
			{
				if (fLoco->Is_Moving_Now())
				{
					x = (float)pType->RollAngle;
				}
				else
				{
					x = pTechno->AngleRotatedSideways;
				}
				// 飞行器的俯仰角度有Techno->AngleRotatedForwards, tt.PitchAngle
				// 根据速度结算
				if (fLoco->TargetSpeed <= pType->PitchSpeed)
				{
					y = pTechno->AngleRotatedForwards;
				}
				else
				{
					y = (float)pType->PitchAngle;
				}
				// 加上姿态的角度
				if (AircraftAttitude* attitude = GetScript<TechnoExt, AircraftAttitude>(pTechno))
				{
					y += attitude->PitchAngle;
				}
			}
			else
			{
				x = pTechno->AngleRotatedSideways;
				y = pTechno->AngleRotatedForwards;
			}
			float scaleY = 1.0;
			float absX = std::abs(x);
			if (absX >= 0.005)
			{
				scaleY = (float)Math::cos(absX);
				pMatrix->ScaleY(scaleY);
			}
			float scaleX = 1.0;
			float absY = std::abs(y);
			if (absY >= 0.005)
			{
				scaleX = (float)Math::cos(absY);
				pMatrix->ScaleX(scaleX);
			}
			if (scaleY != 1.0 || scaleX != 1.0)
			{
				pType->DestroyVoxelShadowCache();
			}
		}
	}
	return 0;
}
#pragma endregion


#pragma region Aircraft Attitude

DEFINE_HOOK(0x4CF80D, FlyLocomotionClass_Draw_Matrix, 0x5)
{
	FlyLocomotionClass* pFly = (FlyLocomotionClass*)(R->ESI() - 4);
	if (TechnoClass* pTechno = pFly->LinkedTo)
	{
		AircraftAttitude* attitude = nullptr;
		if (TryGetScript<TechnoExt, AircraftAttitude>(pTechno, attitude) && attitude->PitchAngle != 0)
		{
			GET_STACK(Matrix3D, matrix3D, 0x8);
			matrix3D.RotateY(attitude->PitchAngle);
			R->Stack(0x8, matrix3D);
		}
	}
	return 0;
}

DEFINE_HOOK(0x4CF4C5, FlyLocomotionClass_FlightLevel_ResetA, 0xA)
{
	GET(FlyLocomotionClass*, pFly, EBP);
	int flightLevel = RulesClass::Instance->FlightLevel;
	if (TechnoClass* pTechno = pFly->LinkedTo)
	{
		AircraftDive* dive = nullptr;
		if (TryGetScript<TechnoExt, AircraftDive>(pTechno, dive)
			&& dive->DiveStatus == AircraftDive::AircraftDiveStatus::DIVEING)
		{
			flightLevel = dive->GetAircraftDiveData()->FlightLevel;
		}
		else
		{
			flightLevel = pTechno->GetTechnoType()->GetFlightLevel();
		}
	}
	R->EAX(flightLevel);
	return 0x4CF4CF;
}

DEFINE_HOOK(0x4CF3E3, FlyLocomotionClass_FlightLevel_ResetB, 0x9)
{
	GET(FlyLocomotionClass*, pFly, EBP);
	if (TechnoClass* pTechno = pFly->LinkedTo)
	{
		AircraftDive* dive = nullptr;
		if (TryGetScript<TechnoExt, AircraftDive>(pTechno, dive)
			&& dive->DiveStatus == AircraftDive::AircraftDiveStatus::DIVEING)
		{
			return 0x4CF4D2;
		}
	}
	return 0;
}

DEFINE_HOOK(0x4CF3C5, FlyLocomotionClass_4CEFB0, 0x6)
{
	// 调整飞机的朝向，有目标时获取目标的朝向，没有目标时获得默认朝向，此时EAX为0
	// EAX是目标DirStruct的指针
	// ECX是当前Facing的指针
	// ESI是飞机的指针的指针
	GET(DirStruct*, pDirEAX, EAX);
	if (!pDirEAX)
	{
		GET(DirStruct*, pDir, EDX);
		GET(TechnoClass**, ppTechno, ESI);
		TechnoClass* pTechno = *ppTechno;
		// 如果是Spawnd就全程强制执行
		// Mission是Enter的普通飞机就不管
		if (pTechno->IsInAir()
			&& (pTechno->GetTechnoType()->Spawned || pTechno->CurrentMission != Mission::Enter))
		{
			pDir->SetValue(pTechno->SecondaryFacing.Current().GetValue());
		}
	}
	return 0;
}

// Hook address form Otamma
DEFINE_HOOK(0x41B760, IFlyControl_Landing_Direction, 0x6)
{
	GET_STACK(IFlyControl*, pAircraft, 0x4); // IFlyControl*
	int poseDir = RulesClass::Instance->PoseDir;
	AircraftAttitude* attitude = nullptr;
	TechnoClass* pTechno = dynamic_cast<AircraftClass*>(pAircraft);
	if (TryGetScript<TechnoExt, AircraftAttitude>(pTechno, attitude)
		&& attitude->TryGetAirportDir(poseDir))
	{
		R->EAX(poseDir);
		return 0x41B7C1;
	}
	return 0;
}

#pragma endregion

#pragma region Aircraft Guard
DEFINE_HOOK(0x41A697, AircraftClass_Mission_Guard_NoTarget_Enter, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AircraftGuard* fighter = nullptr;
	if (TryGetScript<TechnoExt, AircraftGuard>(pTechno, fighter)
		&& fighter->IsAreaGuardRolling())
	{
		// 不返回机场，而是继续前进直到目的地
		return 0x41A6AC;
	}
	return 0;
}

DEFINE_HOOK(0x41A96C, AircraftClass_Mission_GuardArea_NoTarget_Enter, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AircraftGuard* fighter = nullptr;
	if (TryGetScript<TechnoExt, AircraftGuard>(pTechno, fighter))
	{
		// 不返回机场，而是继续前进直到目的地
		fighter->StartAreaGuard();
		return 0x41A97A;
	}
	return 0;
}

DEFINE_HOOK(0x4CF780, FlyLocomotionClass_Draw_Matrix_Rolling, 0x5)
{
	FlyLocomotionClass* pFly = (FlyLocomotionClass*)(R->ESI() - 4);
	TechnoClass* pTechno = pFly->LinkedTo;
	AircraftGuard* fighter = nullptr;
	if (pTechno && pTechno->GetTechnoType()->RollAngle != 0
		&& TryGetScript<TechnoExt, AircraftGuard>(pTechno, fighter)
		&& fighter->State == AircraftGuard::AircraftGuardStatus::ROLLING)
	{
		// 保持倾斜
		if (fighter->Clockwise)
		{
			return 0x4CF7B0; // 右倾
		}
		else
		{
			return 0x4CF7DF; // 左倾
		}
	}
	return 0;
}

#pragma endregion

DEFINE_HOOK(0x418478, AircraftClass_Mission_Attack_Fire_Imcoming_0, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AbstractClass* pTarget = pTechno->Target;
	int weaponIdx = pTechno->SelectWeapon(pTarget);
	WeaponStruct* pWeapon = pTechno->GetWeapon(weaponIdx);
	if (pWeapon && pWeapon->WeaponType && pWeapon->WeaponType->Damage <= 0)
	{
		// skip scatter target cell
		return 0x4184C2;
	}
	return 0;
}

DEFINE_HOOK(0x4186D7, AircraftClass_Mission_Attack_Fire_Imcoming_1, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AbstractClass* pTarget = pTechno->Target;
	int weaponIdx = pTechno->SelectWeapon(pTarget);
	WeaponStruct* pWeapon = pTechno->GetWeapon(weaponIdx);
	if (pWeapon && pWeapon->WeaponType && pWeapon->WeaponType->Damage <= 0)
	{
		// skip scatter target cell
		return 0x418720;
	}
	return 0;
}

DEFINE_HOOK(0x418826, AircraftClass_Mission_Attack_Fire_Imcoming_2, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AbstractClass* pTarget = pTechno->Target;
	int weaponIdx = pTechno->SelectWeapon(pTarget);
	WeaponStruct* pWeapon = pTechno->GetWeapon(weaponIdx);
	if (pWeapon && pWeapon->WeaponType && pWeapon->WeaponType->Damage <= 0)
	{
		// skip scatter target cell
		return 0x418870;
	}
	return 0;
}

DEFINE_HOOK(0x418935, AircraftClass_Mission_Attack_Fire_Imcoming_3, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AbstractClass* pTarget = pTechno->Target;
	int weaponIdx = pTechno->SelectWeapon(pTarget);
	WeaponStruct* pWeapon = pTechno->GetWeapon(weaponIdx);
	if (pWeapon && pWeapon->WeaponType && pWeapon->WeaponType->Damage <= 0)
	{
		// skip scatter target cell
		return 0x41897F;
	}
	return 0;
}

DEFINE_HOOK(0x418A44, AircraftClass_Mission_Attack_Fire_Imcoming_4, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AbstractClass* pTarget = pTechno->Target;
	int weaponIdx = pTechno->SelectWeapon(pTarget);
	WeaponStruct* pWeapon = pTechno->GetWeapon(weaponIdx);
	if (pWeapon && pWeapon->WeaponType && pWeapon->WeaponType->Damage <= 0)
	{
		// skip scatter target cell
		return 0x418A8E;
	}
	return 0;
}

DEFINE_HOOK(0x418B40, AircraftClass_Mission_Attack_Fire_Imcoming_5, 0x6)
{
	GET(TechnoClass*, pTechno, ESI);
	AbstractClass* pTarget = pTechno->Target;
	int weaponIdx = pTechno->SelectWeapon(pTarget);
	WeaponStruct* pWeapon = pTechno->GetWeapon(weaponIdx);
	if (pWeapon && pWeapon->WeaponType && pWeapon->WeaponType->Damage <= 0)
	{
		// skip scatter target cell
		return 0x418B8A;
	}
	return 0;
}


// Aircraft' mission statues:
// 0	INIT_ATTACK				初始状态。检查目标是否存在，设置状态为 10 或 11，返回1
// 1	PICK_ATTACK_LOCATION	选择攻击位置。扣除弹药（如果 TimeStart 标志为真），计算攻击位置并设置目的地。根据是否有伞兵决定下一状态：有伞兵 → 状态3，无伞兵 → 状态10
// 3	APPROACH_TARGET			接近目标。备选弹药扣除点，持续接近目标，检查武器是否就绪（偏移1C函数）。就绪后进入状态4
// 4	FIRING					主开火状态。执行 burst 循环开火（根据武器连发数）。开火后用偏移1C函数检查武器是否就绪：
// 								返回0（未就绪）→ 状态5
// 								返回非0（就绪）→ 继续执行弹药检查，然后根据弹药决定状态1或10
// 5	WAIT_ROF				等待武器冷却。保持朝向目标，持续检查武器状态：
// 								武器就绪 → 再次开火，然后根据 CurleyShuffle 决定下一状态
// 								超出射程 → 根据 CurleyShuffle 决定
// 								默认分支也可能进入状态5（LABEL_54）
// 6	POST_FIRE_1				开火后状态1。执行一次开火，设置状态为7
// 7	POST_FIRE_2				开火后状态2。执行一次开火，设置状态为8
// 8	POST_FIRE_3				开火后状态3。执行一次开火，设置状态为9
// 9	POST_FIRE_FINAL			最终开火状态。执行一次开火，设置状态为3（继续攻击）
// 10	RETREAT					撤退状态。最后弹药扣除点，清除目标，计算撤退位置并前往地图边缘
// 11	UNKNOWN					从状态0计算得来（v3 + 10），可能是特殊攻击模式

// 已经过滤了扫射
DEFINE_HOOK(0x4181CF, AircraftClass_Mission_Attack_FlyToPostion, 0x5)
{
	GET(AircraftClass*, pAir, ESI);
	if (!pAir->Type->MissileSpawn && !pAir->Type->Fighter)
	{
		pAir->MissionStatus = 0x4; // AIR_ATT_FIRE_AT_TARGET0
		return 0x4181E6;
	}
	return 0;
}

namespace AircraftHoverTemp
{
	std::unordered_set<AircraftClass*> Aircraft{};
}

// 飞机在状态4时，检查rof前，决定是否跳转到状态5，做个标记，以便于扣除弹药
DEFINE_HOOK(0x4184FC, AircraftClass_Mission_Attack_CheckROF, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	AircraftHoverTemp::Aircraft.insert(pAir);
	return 0;
}

// 飞机在状态4时，发射武器后，没有进入5，而是直接检查弹药状态
DEFINE_HOOK(0x418506, AircraftClass_Mission_Attack_FireDone, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	AircraftHoverTemp::Aircraft.erase(pAir);
	return 0;
}

DEFINE_HOOK(0x418572, AircraftClass_Mission_Attack_4to5, 0xA)
{
	GET(AircraftClass*, pAir, ESI);
	auto it = AircraftHoverTemp::Aircraft.find(pAir);
	if (it != AircraftHoverTemp::Aircraft.end())
	{
		// 当飞机从4->5时，若CurleyShuffle=yes，5会变成1，1中会扣除弹药，这里不扣除弹药
		if (!GetTypeData<TechnoTypeExt, TechnoTypeExt::TypeData>(pAir->GetTechnoType())->CurleyShuffle)
		{
			pAir->Ammo -= 1;
			pAir->LoseAmmo = false; // 代码在Update和其他Status中通过检查这个来决定是否扣除弹药
		}
		AircraftHoverTemp::Aircraft.erase(it);
	}
	return 0;
}

// 飞机在状态5时，额外发射了一次武器，跳过他
DEFINE_HOOK(0x4186B6, AircraftClass_Mission_Attack5_HoverFireTwice, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	if (GetTypeData<TechnoTypeExt, TechnoTypeExt::TypeData>(pAir->GetTechnoType())->DisableNoFighterFireTwice)
	{
		return 0x4186D7;
	}
	return 0;
}

DEFINE_HOOK(0x4183D3, AircraftClass_Mission_Attack4_HoverFireMove, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	if (GetTypeData<TechnoTypeExt, TechnoTypeExt::TypeData>(pAir->GetTechnoType())->CurleyShuffle)
	{
		// this->t.r.m.MissionStatus = RulesData->CurleyShuffle != 0 ? 1 : 4;
		R->EDX(0x1);
	}
	else
	{
		R->EDX(0x4); // AIR_ATT_FIRE_AT_TARGET0
	}
		return 0;
}

DEFINE_HOOK(0x418743, AircraftClass_Mission_Attack5_HoverFireMove_FireError_0, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	if (GetTypeData<TechnoTypeExt, TechnoTypeExt::TypeData>(pAir->GetTechnoType())->CurleyShuffle)
	{
		// this->t.r.m.MissionStatus = RulesData->CurleyShuffle != 0 ? 1 : 4;
		R->ECX(0x1);
	}
	else
	{
		R->ECX(0x4); // AIR_ATT_FIRE_AT_TARGET0
	}
	return 0;
}

DEFINE_HOOK(0x418680, AircraftClass_Mission_Attack5_HoverFireMove_FireError_2, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	if (GetTypeData<TechnoTypeExt, TechnoTypeExt::TypeData>(pAir->GetTechnoType())->CurleyShuffle)
	{
		// this->t.r.m.MissionStatus = RulesData->CurleyShuffle != 0 ? 1 : 4;
		R->EAX(0x1);
	}
	else
	{
		R->EAX(0x4); // AIR_ATT_FIRE_AT_TARGET0
	}
	return 0;
}

DEFINE_HOOK(0x418792, AircraftClass_Mission_Attack5_HoverFireMove_FireError_def, 0x6)
{
	GET(AircraftClass*, pAir, ESI);
	if (GetTypeData<TechnoTypeExt, TechnoTypeExt::TypeData>(pAir->GetTechnoType())->CurleyShuffle)
	{
		// this->t.r.m.MissionStatus = RulesData->CurleyShuffle != 0 ? 1 : 4;
		R->EDX(0x1);
	}
	else
	{
		R->EDX(0x4); // AIR_ATT_FIRE_AT_TARGET0
	}
	return 0;
}


// Aircrate hover attack
DEFINE_HOOK(0x4CDCFD, FlyLocomotionClass_MovingUpdate_HoverAttack, 0x7)
{
	GET(FlyLocomotionClass*, pFly, ESI);
	AircraftClass* pAir = abstract_cast<AircraftClass*>(pFly->LinkedTo);
	if (pAir && !pAir->Type->MissileSpawn && !pAir->Type->Fighter && !pAir->Is_Strafe() && pAir->CurrentMission == Mission::Attack)
	{
		if (AbstractClass* pDest = pAir->Destination)
		{
			CoordStruct sourcePos = pAir->GetCoords();
			int dist = pAir->DistanceFrom(pDest);
			// 进入开火位置的判定距离是16，有时候距离50就可以开火
			if (dist < 64 && dist >= 16)
			{
				CoordStruct targetPos = pDest->GetCoords();
				sourcePos.X = targetPos.X;
				sourcePos.Y = targetPos.Y;
				dist = 0;
			}
			if (dist < 16)
			{
				// 固定位置不动
				R->Stack(0x50, sourcePos);
			}
		}
	}
	return 0;
}


DEFINE_HOOK(0x4DDD66, FootClass_IsLandZoneClear_ReplaceHardcode, 0x6) // To avoid that the aircraft cannot fly towards the water surface normally
{
	enum { SkipGameCode = 0x4DDD8A };

	GET(FootClass* const, pThis, EBP);
	GET_STACK(const CellStruct, cell, STACK_OFFSET(0x20, 0x4));

	const auto pType = pThis->GetTechnoType();

	// In vanilla, only aircrafts or `foots with fly locomotion` will call this virtual function
	// So I don't know why WW use hard-coded `SpeedType::Track` and `MovementZone::Normal` to check this
	R->AL(MapClass::Instance->GetCellAt(cell)->IsClearToMove(pType->SpeedType, false, false, ZoneType::None, pType->MovementZone, -1, true));
	return SkipGameCode;
}
