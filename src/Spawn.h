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

	// Optional inheritance applied to each released unit, sourced from the
	// spawning host/box. Health/veterancy are safe for any source; passengers
	// only make sense when the source is consumed (a GiftBox), and transfer up to
	// the gift's own passenger capacity.
	struct InheritSpec
	{
		TechnoClass* source = nullptr;
		bool health = false;
		double healthPercent = 0.0;   // 0 = copy the source's current health %
		bool veterancy = false;
		bool passengers = false;
	};

	// Place each gift near `pSource` (scattered up to `range` cells), marking them
	// gift-spawned and applying `inherit`. Returns the number successfully placed.
	// Uses the engine's own cell for the source (big-map safe, unlike YRpp's
	// hardcoded-512-stride cell lookups). Falls back to the source cell so a gift
	// always lands somewhere valid.
	int ReleaseList(const std::vector<TechnoTypeClass*>& gifts, HouseClass* pHouse,
		TechnoClass* pSource, int range, const InheritSpec& inherit);

	// Chain-guard registry: a unit produced by Host/GiftBox is remembered so that,
	// under OnlyBuilt, it will not itself spawn.
	void MarkGiftSpawned(TechnoClass* pTechno);
	bool IsGiftSpawned(TechnoClass* pTechno);
	void Forget(TechnoClass* pTechno);   // called from the TechnoClass destructor hook
}
