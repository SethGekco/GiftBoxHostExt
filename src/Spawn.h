// Shared spawn machinery used by both Host and GiftBox:
//  - cell placement (scatter within a range, prefer clear cells)
//  - the "gift-spawned" registry that powers the OnlyBuilt chain-guard
// All randomness uses the game's synchronized RNG, so it is netplay-safe.
#pragma once

#include <GeneralStructures.h>   // CoordStruct
#include <string>
#include <vector>

class TechnoClass;
class TechnoTypeClass;
class HouseClass;
class CellClass;
struct IStream;

namespace GiftBoxHost::Spawn
{
	// Save/load the gift-spawned registry (appended to the savegame stream).
	void SaveState(IStream* stream);
	void LoadState(IStream* stream);

	// Resolve the concrete list of gift types to spawn this trigger, applying
	// RandomType weighting (RandomWeights) and per-type Chances via the synced RNG.
	//  - randomType=false: each type i spawned nums[i] times (default 1), gated by chances[i].
	//  - randomType=true : sum(nums) picks from the weighted type list, each gated by chances.
	std::vector<TechnoTypeClass*> BuildGiftList(
		const std::vector<std::string>& types,
		const std::vector<int>& nums,
		const std::vector<double>& chances,
		bool randomType,
		const std::vector<int>& weights);

	// Place each type in `gifts` near origin (scattered), marking them gift-spawned.
	// Returns the number successfully placed.
	int ReleaseList(const std::vector<TechnoTypeClass*>& gifts, HouseClass* pHouse,
		CoordStruct origin, int range, bool emptyCell);

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
