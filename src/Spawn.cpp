#include "Spawn.h"
#include "Serialize.h"
#include "Log.h"

#include <GeneralDefinitions.h>   // DirType
#include <FootClass.h>            // passenger transfer (FootClass*), cast-trait completeness
#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <HouseClass.h>
#include <MapClass.h>
#include <CellClass.h>
#include <ScenarioClass.h>
#include <Unsorted.h>

#include <unordered_set>
#include <utility>

namespace GiftBoxHost::Spawn
{
	// Units a factory has kicked out (= "built"). Only ever queried by pointer
	// (never iterated for game logic), so pointer keys are deterministic /
	// netplay-safe. Persisted across savegames; cleared on unit death.
	static std::unordered_set<TechnoClass*> g_built;

	// Big-map-safe placement. YRpp's inline cell-array indexing (GetCellIndex /
	// TryGetCellAt / Coord2Cell) hardcodes the vanilla 512-cell stride, so on
	// MapSizeExt-expanded maps it returns the wrong (or a null) cell. We instead
	// take the source's OWN cell from the engine (GetCell, a virtual that uses the
	// real map) and scatter by world-coordinate offset (256 leptons per cell),
	// letting Unlimbo validate the final placement.
	static CoordStruct SpawnBaseCoord(TechnoClass* pSource)
	{
		if (CellClass* pCell = pSource->GetCell())
			return pCell->GetCoordsWithBridge();
		return pSource->GetCoords();
	}

	// Create one gift and place it near `base`, scattered up to `range` cells.
	// Tries several scattered spots, then falls back to the base cell so a gift
	// always lands somewhere valid. Returns nullptr only if even the base fails.
	static TechnoClass* PlaceGift(TechnoTypeClass* pType, HouseClass* pHouse, CoordStruct base, int range)
	{
		TechnoClass* pGift = static_cast<TechnoClass*>(pType->CreateObject(pHouse));
		if (!pGift)
			return nullptr;

		int scatterTries = range > 0 ? 8 : 0;
		for (int t = 0; t <= scatterTries; ++t)
		{
			CoordStruct coords = base;
			if (t < scatterTries) // last iteration always uses the base cell
			{
				int dx = ScenarioClass::Instance->Random.RandomRanged(0, 2 * range) - range;
				int dy = ScenarioClass::Instance->Random.RandomRanged(0, 2 * range) - range;
				coords.X += dx * 256;
				coords.Y += dy * 256;
			}
			++Unsorted::IKnowWhatImDoing;
			bool placed = pGift->Unlimbo(coords, DirType::East);
			--Unsorted::IKnowWhatImDoing;
			if (placed)
			{
				pGift->SetLocation(coords);
				return pGift;
			}
		}
		return nullptr; // could not place even at the source cell (very rare)
	}

	static void ApplyInherit(TechnoClass* pGift, const InheritSpec& in)
	{
		if (!in.source)
			return;

		if (in.health)
		{
			double pct = in.healthPercent > 0.0 ? in.healthPercent : in.source->GetHealthPercentage();
			if (pct <= 0.0) pct = 1.0;
			if (pct > 1.0) pct = 1.0;
			int hp = static_cast<int>(pGift->GetTechnoType()->Strength * pct);
			if (hp < 1) hp = 1;
			pGift->Health = hp;
		}

		if (in.veterancy)
			pGift->Veterancy = in.source->Veterancy;

		if (in.passengers)
		{
			int cap = pGift->GetTechnoType()->Passengers;
			while (in.source->Passengers.NumPassengers > 0 && pGift->Passengers.NumPassengers < cap)
			{
				FootClass* pPassenger = in.source->Passengers.RemoveFirstPassenger();
				if (!pPassenger)
					break;
				pGift->AddPassenger(pPassenger);
			}
		}
	}

	int ReleaseList(const std::vector<TechnoTypeClass*>& gifts, HouseClass* pHouse,
		TechnoClass* pSource, int range, const InheritSpec& inherit)
	{
		if (!pSource)
			return 0;
		int ok = 0;
		CoordStruct base = SpawnBaseCoord(pSource);
		for (TechnoTypeClass* pType : gifts)
		{
			if (TechnoClass* pGift = PlaceGift(pType, pHouse, base, range))
			{
				// Deliberately NOT marked built — a spawned gift never went through
				// a factory, so under OnlyBuilt it won't Host/open (chain guard).
				ApplyInherit(pGift, inherit);
				++ok;
			}
		}
		return ok;
	}

	// ---- gift-list resolution (weighting + chances, synced RNG) --------------
	static bool Bingo(const std::vector<double>& chances, int index)
	{
		if (static_cast<int>(chances.size()) < index + 1)
			return true; // no chance given for this index -> always
		double c = chances[index];
		if (c <= 0.0) return false;
		if (c >= 1.0) return true;
		double roll = ScenarioClass::Instance->Random.RandomRanged(0, 9999) / 10000.0; // [0,1)
		return c > roll;
	}

	std::vector<TechnoTypeClass*> BuildGiftList(
		const std::vector<std::string>& types,
		const std::vector<int>& nums,
		const std::vector<double>& chances,
		bool randomType,
		const std::vector<int>& weights)
	{
		std::vector<TechnoTypeClass*> out;
		int typeCount = static_cast<int>(types.size());
		if (typeCount == 0)
			return out;

		auto resolve = [](const std::string& id) -> TechnoTypeClass* {
			return TechnoTypeClass::Find(id.c_str());
		};

		if (randomType)
		{
			// total picks = sum(nums) (or 1 if no nums)
			int times = 1;
			if (!nums.empty())
			{
				times = 0;
				for (int n : nums) times += n;
			}
			// cumulative weight ranges: index i owns [lo, hi)
			int maxValue = 0;
			std::vector<std::pair<int, int>> pad;
			pad.reserve(typeCount);
			for (int i = 0; i < typeCount; ++i)
			{
				int lo = maxValue;
				int w = (static_cast<int>(weights.size()) > i && weights[i] > 0) ? weights[i] : 1;
				maxValue += w;
				pad.emplace_back(lo, maxValue);
			}
			for (int t = 0; t < times; ++t)
			{
				int index = 0;
				if (maxValue > 0)
				{
					int p = ScenarioClass::Instance->Random.RandomRanged(0, maxValue - 1);
					for (int i = 0; i < typeCount; ++i)
						if (p >= pad[i].first && p < pad[i].second) { index = i; break; }
				}
				if (Bingo(chances, index))
					if (TechnoTypeClass* pType = resolve(types[index]))
						out.push_back(pType);
			}
		}
		else
		{
			for (int i = 0; i < typeCount; ++i)
			{
				int count = (static_cast<int>(nums.size()) > i && nums[i] > 0) ? nums[i] : 1;
				for (int c = 0; c < count; ++c)
					if (Bingo(chances, i))
						if (TechnoTypeClass* pType = resolve(types[i]))
							out.push_back(pType);
			}
		}
		return out;
	}

	void MarkBuilt(TechnoClass* pTechno) { if (pTechno) g_built.insert(pTechno); }
	bool IsBuilt(TechnoClass* pTechno) { return g_built.count(pTechno) != 0; }
	void Forget(TechnoClass* pTechno) { g_built.erase(pTechno); }

	void SaveState(IStream* stream)
	{
		unsigned count = static_cast<unsigned>(g_built.size());
		Serialize::Write(stream, count);
		for (TechnoClass* p : g_built)
			Serialize::WritePtr(stream, p);
	}

	void LoadState(IStream* stream)
	{
		g_built.clear();
		unsigned count = 0;
		if (!Serialize::Read(stream, count))
			return;
		for (unsigned i = 0; i < count; ++i)
			if (void* p = Serialize::ReadSwizzled(stream))
				g_built.insert(static_cast<TechnoClass*>(p));
	}
}
