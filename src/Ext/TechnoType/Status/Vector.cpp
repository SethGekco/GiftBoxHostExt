#include "../TechnoStatus.h"

#include <JumpjetLocomotionClass.h>

#include <Ext/Helper/FLH.h>
#include <Ext/Helper/Physics.h>
#include <Ext/Helper/Weapon.h>
#include <Ext/Helper/Scripts.h>

void TechnoStatus::VectorCancel()
{
	if (!IsBuilding() && !IsDeadOrInvisible(pTechno))
	{
		FootClass* pFoot = abstract_cast<FootClass*, true>(pTechno);
		pFoot->Locomotor->Unlock();
		// 恢复可控制
		if (_lostControl)
		{
			pFoot->ForceMission(Mission::Guard);
		}
		// 坠落：取当前高度对地差值或 AllowFallingDestroy 指定高度
		CellClass* pCell = MapClass::Instance->TryGetCellAt(pTechno->GetCoords());
		int heightAboveGround = pCell ? (pTechno->GetCoords().Z - pCell->GetCoordsWithBridge().Z) : 0;
		int fallingDestroyHeight = _vectorResult.AllowFallingDestroy
			? _vectorResult.FallingDestroyHeight
			: heightAboveGround;
		FallingExceptAircraft(pTechno, fallingDestroyHeight, false);
	}
	VectorForced = false;
	_vectorResult = {};
}


void TechnoStatus::OnUpdate_Vector()
{
	if (CaptureByBlackHole || IsBuilding() || IsDeadOrInvisible(pTechno))
		return;

	bool wasVectorForced = VectorForced;

	_vectorResult = AEManager()->MarginVectorOffset();

	if (!_vectorResult.MoveDisp.IsEmpty())
	{
		VectorForced = true;
		VectorPendingFall = false;
		CoordStruct location = pTechno->GetCoords();

		if (IsDeadOrInvisible(pTechno))
		{
			VectorCancel();
		}
		else
		{
			// 移动位置
			// 从占据的格子中移除自己
			pTechno->UnmarkAllOccupationBits(location);
			FootClass* pFoot = abstract_cast<FootClass*, true>(pTechno);
			// 停止移动
			ForceStopMoving(pFoot);
			// 计算下一个坐标点
			CoordStruct nextPos = location + _vectorResult.MoveDisp;
			CoordStruct nextCellPos = CoordStruct::Empty;
			bool onBridge = pTechno->OnBridge;
			PassError passError = CanMoveTo(location, nextPos, _vectorResult.CanPassBuilding, nextCellPos, onBridge);
			switch (passError)
			{
			case PassError::HITWALL:
			case PassError::HITBUILDING:
			case PassError::UPBRIDEG:
				// 反弹回移动前的格子
				if (CellClass* pSourceCell = MapClass::Instance->TryGetCellAt(location))
				{
					CoordStruct cellPos = pSourceCell->GetCoordsWithBridge();
					nextPos.X = cellPos.X;
					nextPos.Y = cellPos.Y;
					if (nextPos.Z < cellPos.Z)
					{
						nextPos.Z = cellPos.Z;
					}
				}
				break;
			case PassError::UNDERGROUND:
			case PassError::DOWNBRIDGE:
				// 卡在地表
				nextPos.Z = nextCellPos.Z;
				break;
			}
			// 被黑洞吸走
			pTechno->UpdatePlacement(PlacementType::Remove);
			// 是否在桥上
			pTechno->OnBridge = onBridge;
			pTechno->SetLocation(nextPos);
			pTechno->UpdatePlacement(PlacementType::Put);
			// 移除黑幕
			MapClass::Instance->RevealArea2(&nextPos, pTechno->LastSightRange, pTechno->Owner, false, false, false, true, 0);
			MapClass::Instance->RevealArea2(&nextPos, pTechno->LastSightRange, pTechno->Owner, false, false, false, true, 1);
			// 设置动作
			if (_vectorResult.AllowCrawl && IsInfantry())
			{
				abstract_cast<InfantryClass*, true>(pTechno)->PlayAnim(Sequence::Crawl);
			}
			// 设置翻滚
			if (_vectorResult.AllowRotateUnit)
			{
				// 设置朝向
				if (_lastMission == Mission::Move || _lastMission == Mission::AttackMove || pTechno->GetTechnoType()->ConsideredAircraft || !pTechno->IsInAir())
				{
					CoordStruct p1 = nextPos;
					CoordStruct p2 = location;
					p1.Z = 0;
					p2.Z = 0;
					if (p1.DistanceFrom(p2) >= Unsorted::LeptonsPerCell * 0.5)
					{
						DirStruct facingDir = Point2Dir(nextPos, location);
						pTechno->PrimaryFacing.SetDesired(facingDir);
						if (IsJumpjet())
						{
							if (JumpjetLocomotionClass* jjLoco = dynamic_cast<JumpjetLocomotionClass*>(pFoot->Locomotor.get()))
							{
								jjLoco->LocomotionFacing.SetDesired(facingDir);
							}
						}
						else if (IsAircraft())
						{
							pTechno->SecondaryFacing.SetDesired(facingDir);
						}
					}
				}
			}
		}
	}
	else if (wasVectorForced)
	{
		// 延后 1 帧判定坠落，等待 Next= 链条衔接
		VectorPendingFall = true;
		VectorForced = false;
	}
	else if (VectorPendingFall)
	{
		VectorCancel();
		VectorPendingFall = false;
	}
}

