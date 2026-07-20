#include "Spawn.h"

#include <GeneralDefinitions.h>   // DirType
#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <HouseClass.h>
#include <MapClass.h>
#include <CellClass.h>
#include <ScenarioClass.h>
#include <Unsorted.h>

#include <unordered_set>

namespace GiftBoxHost::Spawn
{
	// Units produced by a Host/GiftBox. Only ever queried by pointer (never
	// iterated for game logic), so pointer keys are deterministic / netplay-safe.
	static std::unordered_set<TechnoClass*> g_giftSpawned;

	CellClass* PickCell(TechnoTypeClass* pType, CoordStruct origin, int range, bool emptyCell)
	{
		CellClass* pCenter = MapClass::Instance->TryGetCellAt(origin);
		if (!pCenter || range <= 0)
			return pCenter;

		CellStruct center = pCenter->MapCoords;
		int attempts = (2 * range + 1) * (2 * range + 1);
		for (int i = 0; i < attempts; ++i)
		{
			int dx = ScenarioClass::Instance->Random.RandomRanged(-range, range);
			int dy = ScenarioClass::Instance->Random.RandomRanged(-range, range);
			CellStruct pos{ static_cast<short>(center.X + dx), static_cast<short>(center.Y + dy) };
			if (CellClass* pCell = MapClass::Instance->TryGetCellAt(pos))
			{
				if (pCell->IsClearToMove(pType->SpeedType, pType->MovementZone, !emptyCell, !emptyCell))
					return pCell;
			}
		}
		return pCenter; // nothing clear in range: fall back to the origin cell
	}

	static bool TryPut(TechnoClass* pTechno, CellClass* pCell)
	{
		if (!pCell)
			return false;

		pTechno->OnBridge = pCell->ContainsBridge();
		CoordStruct xyz = pCell->GetCoordsWithBridge();

		++Unsorted::IKnowWhatImDoing;
		pTechno->Unlimbo(xyz, DirType::East);
		--Unsorted::IKnowWhatImDoing;

		pTechno->SetLocation(xyz);
		return true;
	}

	TechnoClass* CreateAndPut(TechnoTypeClass* pType, HouseClass* pHouse, CellClass* pCell)
	{
		// CreateObject for a TechnoType yields a TechnoClass-derived object
		// (single, non-virtual inheritance chain), so this downcast is valid.
		TechnoClass* pTechno = static_cast<TechnoClass*>(pType->CreateObject(pHouse));
		if (pTechno && TryPut(pTechno, pCell))
			return pTechno;
		return nullptr;
	}

	int Release(TechnoTypeClass* pType, HouseClass* pHouse, CoordStruct origin,
		int count, int range, bool emptyCell)
	{
		int ok = 0;
		for (int c = 0; c < count; ++c)
		{
			CellClass* pCell = PickCell(pType, origin, range, emptyCell);
			if (TechnoClass* pGift = CreateAndPut(pType, pHouse, pCell))
			{
				MarkGiftSpawned(pGift);
				++ok;
			}
		}
		return ok;
	}

	void MarkGiftSpawned(TechnoClass* pTechno) { g_giftSpawned.insert(pTechno); }
	bool IsGiftSpawned(TechnoClass* pTechno) { return g_giftSpawned.count(pTechno) != 0; }
	void Forget(TechnoClass* pTechno) { g_giftSpawned.erase(pTechno); }
}
