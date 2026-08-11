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
	// Units produced by a Host/GiftBox. Only ever queried by pointer (never
	// iterated for game logic), so pointer keys are deterministic / netplay-safe.
	static std::unordered_set<TechnoClass*> g_giftSpawned;

	CellClass* PickCell(TechnoTypeClass* pType, CoordStruct origin, int range, bool emptyCell)
	{
		CellClass* pCenter = MapClass::Instance->TryGetCellAt(origin);
		if (!pCenter || range <= 0)
			return pCenter;

		CellStruct center = pCenter->MapCoords;

		// Candidate offsets within the range box (Kratos-style: pick a random offset
		// index each try, add to the center cell via CellStruct operator+).
		std::vector<CellStruct> offsets;
		for (short ox = static_cast<short>(-range); ox <= static_cast<short>(range); ++ox)
			for (short oy = static_cast<short>(-range); oy <= static_cast<short>(range); ++oy)
				offsets.push_back(CellStruct{ ox, oy });

		int count = static_cast<int>(offsets.size());
		for (int i = 0; i < count; ++i)
		{
			int idx = ScenarioClass::Instance->Random.RandomRanged(0, count - 1);
			CellStruct target = center + offsets[idx];
			if (CellClass* pCell = MapClass::Instance->TryGetCellAt(target))
			{
				if (pCell->IsClearToMove(pType->SpeedType, pType->MovementZone, !emptyCell, !emptyCell))
				{
					Log("[Spawn] center(%d,%d) off(%d,%d) -> cell(%d,%d)",
						center.X, center.Y, offsets[idx].X, offsets[idx].Y,
						pCell->MapCoords.X, pCell->MapCoords.Y);
					return pCell;
				}
			}
		}
		Log("[Spawn] center(%d,%d) no clear cell in range %d -> fallback", center.X, center.Y, range);
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
		CoordStruct origin, int range, bool emptyCell, const InheritSpec& inherit)
	{
		int ok = 0;
		for (TechnoTypeClass* pType : gifts)
		{
			CellClass* pCell = PickCell(pType, origin, range, emptyCell);
			if (TechnoClass* pGift = CreateAndPut(pType, pHouse, pCell))
			{
				MarkGiftSpawned(pGift);
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

	void MarkGiftSpawned(TechnoClass* pTechno) { g_giftSpawned.insert(pTechno); }
	bool IsGiftSpawned(TechnoClass* pTechno) { return g_giftSpawned.count(pTechno) != 0; }
	void Forget(TechnoClass* pTechno) { g_giftSpawned.erase(pTechno); }

	void SaveState(IStream* stream)
	{
		unsigned count = static_cast<unsigned>(g_giftSpawned.size());
		Serialize::Write(stream, count);
		for (TechnoClass* p : g_giftSpawned)
			Serialize::WritePtr(stream, p);
	}

	void LoadState(IStream* stream)
	{
		g_giftSpawned.clear();
		unsigned count = 0;
		if (!Serialize::Read(stream, count))
			return;
		for (unsigned i = 0; i < count; ++i)
			if (void* p = Serialize::ReadSwizzled(stream))
				g_giftSpawned.insert(static_cast<TechnoClass*>(p));
	}
}
