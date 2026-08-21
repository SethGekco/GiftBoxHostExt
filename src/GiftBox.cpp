#include "GiftBox.h"
#include "Spawn.h"
#include "Ini.h"
#include "Serialize.h"
#include "Log.h"

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <CCINIClass.h>

#include <unordered_map>

namespace GiftBoxHost
{
	static std::unordered_map<TechnoTypeClass*, GiftBoxConfig> g_cfgs;
	static std::unordered_map<TechnoClass*, GiftBoxState> g_states;

	const GiftBoxConfig& GetGiftBoxConfig(TechnoTypeClass* pType)
	{
		auto it = g_cfgs.find(pType);
		if (it != g_cfgs.end())
			return it->second;

		GiftBoxConfig cfg;
		CCINIClass* pINI = CCINIClass::INI_Rules;
		const char* s = pType->ID;
		char buf[256];

		pINI->ReadString(s, "GiftBox.Types", "", buf, sizeof(buf));
		cfg.types = Ini::SplitList(buf);
		if (!cfg.types.empty())
		{
			cfg.enabled = true;

			pINI->ReadString(s, "GiftBox.Nums", "", buf, sizeof(buf));
			cfg.nums = Ini::SplitInts(buf);

			cfg.delay = pINI->ReadInteger(s, "GiftBox.Delay", 0);
			cfg.initialDelay = pINI->ReadInteger(s, "GiftBox.InitialDelay", 0);
			cfg.randomRange = pINI->ReadInteger(s, "GiftBox.RandomRange", 0);
			cfg.emptyCell = pINI->ReadBool(s, "GiftBox.RandomToEmptyCell", true);
			cfg.onlyBuilt = pINI->ReadBool(s, "GiftBox.OnlyBuilt", false);
			cfg.openWhenDestroyed = pINI->ReadBool(s, "GiftBox.OpenWhenDestroyed", false);
			cfg.remove = pINI->ReadBool(s, "GiftBox.Remove", true);
			cfg.explodes = pINI->ReadBool(s, "GiftBox.Explodes", false);
			cfg.randomType = pINI->ReadBool(s, "GiftBox.RandomType", false);

			pINI->ReadString(s, "GiftBox.RandomWeights", "", buf, sizeof(buf));
			cfg.weights = Ini::SplitInts(buf);
			pINI->ReadString(s, "GiftBox.Chances", "", buf, sizeof(buf));
			cfg.chances = Ini::SplitChances(buf);

			cfg.inheritHealth = pINI->ReadBool(s, "GiftBox.InheritHealth", false);
			cfg.healthPercent = pINI->ReadDouble(s, "GiftBox.HealthPercent", 0.0);
			cfg.inheritVeterancy = pINI->ReadBool(s, "GiftBox.InheritVeterancy", false);
			cfg.inheritPassenger = pINI->ReadBool(s, "GiftBox.InheritPassenger", false);

			pINI->ReadString(s, "GiftBox.RandomDelay", "", buf, sizeof(buf));
			std::vector<int> rd = Ini::SplitInts(buf);
			if (rd.size() >= 2) { cfg.delayMin = rd[0]; cfg.delayMax = rd[1]; }
		}

		return g_cfgs.emplace(pType, std::move(cfg)).first->second;
	}

	static int NextDelay(const GiftBoxConfig& c)
	{
		if (c.delayMax > c.delayMin && c.delayMax > 0)
			return ScenarioClass::Instance->Random.RandomRanged(c.delayMin, c.delayMax);
		return c.delay > 0 ? c.delay : 0;
	}

	static void ReleaseAll(TechnoClass* pBox, const GiftBoxConfig& cfg, const char* why)
	{
		HouseClass* pHouse = pBox->Owner;
		CoordStruct origin = pBox->GetCoords();
		TechnoTypeClass* pBoxType = pBox->GetTechnoType();
		auto gifts = Spawn::BuildGiftList(cfg.types, cfg.nums, cfg.chances, cfg.randomType, cfg.weights);
		Spawn::InheritSpec inherit;
		inherit.source = pBox;
		inherit.health = cfg.inheritHealth || cfg.healthPercent > 0.0;
		inherit.healthPercent = cfg.healthPercent;
		inherit.veterancy = cfg.inheritVeterancy;
		inherit.passengers = cfg.inheritPassenger;
		int ok = Spawn::ReleaseList(gifts, pHouse, pBox, cfg.randomRange, inherit);
		Log("[GiftBox] %s open(%s): released %d/%d at (%d,%d,%d)",
			pBoxType->ID, why, ok, (int)gifts.size(), origin.X, origin.Y, origin.Z);
	}

	void ForgetGiftBox(TechnoClass* pTechno) { g_states.erase(pTechno); }

	void SaveGiftBoxState(IStream* stream)
	{
		unsigned count = static_cast<unsigned>(g_states.size());
		Serialize::Write(stream, count);
		for (auto& kv : g_states)
		{
			Serialize::WritePtr(stream, kv.first);
			Serialize::Write(stream, kv.second);
		}
	}

	void LoadGiftBoxState(IStream* stream)
	{
		g_states.clear();
		unsigned count = 0;
		if (!Serialize::Read(stream, count))
			return;
		for (unsigned i = 0; i < count; ++i)
		{
			void* p = Serialize::ReadSwizzled(stream);
			GiftBoxState st;
			if (!Serialize::Read(stream, st))
				return;
			if (p)
				g_states[static_cast<TechnoClass*>(p)] = st;
		}
	}

	void UpdateGiftBox(TechnoClass* pTechno)
	{
		TechnoTypeClass* pType = pTechno->GetTechnoType();
		if (!pType)
			return;

		const GiftBoxConfig& cfg = GetGiftBoxConfig(pType);
		if (!cfg.enabled || cfg.openWhenDestroyed)   // death-triggered boxes handled in GiftBoxOnDestroyed
			return;
		if (cfg.onlyBuilt && Spawn::IsGiftSpawned(pTechno))
			return;

		GiftBoxState& st = g_states[pTechno];
		if (st.opened)
			return;

		if (!st.initialized)
		{
			st.initialized = true;
			st.timer = cfg.initialDelay > 0 ? cfg.initialDelay : NextDelay(cfg);
		}

		if (st.timer > 0)
		{
			--st.timer;
			return;
		}

		ReleaseAll(pTechno, cfg, "timer");

		if (cfg.remove)
		{
			st.opened = true;
			// Consume the box via the game's damage/death path (safe to call during
			// this unit's own update). Explodes-vs-silent styling is future work.
			pTechno->TakeDamage(pTechno->Health + 1, pType->Crewed);
		}
		else
		{
			st.timer = NextDelay(cfg); // repeat
		}
	}

	void GiftBoxOnDestroyed(TechnoClass* pTechno)
	{
		TechnoTypeClass* pType = pTechno->GetTechnoType();
		if (!pType)
			return;

		const GiftBoxConfig& cfg = GetGiftBoxConfig(pType);
		if (!cfg.enabled || !cfg.openWhenDestroyed)
			return;
		if (cfg.onlyBuilt && Spawn::IsGiftSpawned(pTechno))
			return;

		GiftBoxState& st = g_states[pTechno];
		if (st.opened)
			return;
		st.opened = true;

		ReleaseAll(pTechno, cfg, "destroyed");
	}
}
