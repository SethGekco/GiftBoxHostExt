// Shared spawn machinery used by both Host and GiftBox:
//  - cell placement (scatter within a range, prefer clear cells)
//  - the "gift-spawned" registry that powers the OnlyBuilt chain-guard
// All randomness uses the game's synchronized RNG, so it is netplay-safe.
#pragma once

#include <GeneralStructures.h>   // CoordStruct

class TechnoClass;
class TechnoTypeClass;
class HouseClass;
class CellClass;

namespace GiftBoxHost::Spawn
{
	// Pick a placement cell near `origin`, within `range` cells. When emptyCell is
	// true, prefers a cell the type can actually stand on (spreads a burst out).
	CellClass* PickCell(TechnoTypeClass* pType, CoordStruct origin, int range, bool emptyCell);

	// Create one unit of pType for pHouse and place it on pCell. Returns it (marked
	// gift-spawned by the caller via Release), or nullptr on failure.
	TechnoClass* CreateAndPut(TechnoTypeClass* pType, HouseClass* pHouse, CellClass* pCell);

	// Spawn `count` of pType near origin, marking each as gift-spawned. Returns the
	// number successfully placed.
	int Release(TechnoTypeClass* pType, HouseClass* pHouse, CoordStruct origin,
		int count, int range, bool emptyCell);

	// Chain-guard registry: a unit produced by Host/GiftBox is remembered so that,
	// under OnlyBuilt, it will not itself spawn.
	void MarkGiftSpawned(TechnoClass* pTechno);
	bool IsGiftSpawned(TechnoClass* pTechno);
	void Forget(TechnoClass* pTechno);   // called from the TechnoClass destructor hook
}
