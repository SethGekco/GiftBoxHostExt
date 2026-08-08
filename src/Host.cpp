#include "Host.h"
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
	static std::unordered_map<TechnoTypeClass*, HostConfig> g_configs;
	static std::unordered_map<TechnoClass*, HostState> g_states;

	const HostConfig& GetHostConfig(TechnoTypeClass* pType)
	{
		auto it = g_configs.find(pType);
		if (it != g_configs.end())
			return it->second;

		HostConfig cfg;
		CCINIClass* pINI = CCINIClass::INI_Rules;
		const char* section = pType->ID;
		char buf[256];

		pINI->ReadString(section, "Host.Types", "", buf, sizeof(buf));
		cfg.types = Ini::SplitList(buf);
		if (!cfg.types.empty())
		{
			cfg.enabled = true;

			pINI->ReadString(section, "Host.Nums", "", buf, sizeof(buf));
			cfg.nums = Ini::SplitInts(buf);

			cfg.delay = pINI->ReadInteger(section, "Host.Delay", 0);
			cfg.initialDelay = pINI->ReadInteger(section, "Host.InitialDelay", 0);
			cfg.triggeredTimes = pINI->ReadInteger(section, "Host.TriggeredTimes", 0);
			cfg.randomRange = pINI->ReadInteger(section, "Host.RandomRange", 0);
			cfg.emptyCell = pINI->ReadBool(section, "Host.RandomToEmptyCell", true);
			cfg.onlyBuilt = pINI->ReadBool(section, "Host.OnlyBuilt", false);
			cfg.randomType = pINI->ReadBool(section, "Host.RandomType", false);

			pINI->ReadString(section, "Host.RandomWeights", "", buf, sizeof(buf));
			cfg.weights = Ini::SplitInts(buf);
			pINI->ReadString(section, "Host.Chances", "", buf, sizeof(buf));
			cfg.chances = Ini::SplitChances(buf);

			cfg.inheritHealth = pINI->ReadBool(section, "Host.InheritHealth", false);
			cfg.healthPercent = pINI->ReadDouble(section, "Host.HealthPercent", 0.0);
			cfg.inheritVeterancy = pINI->ReadBool(section, "Host.InheritVeterancy", false);

			pINI->ReadString(section, "Host.RandomDelay", "", buf, sizeof(buf));
			std::vector<int> rd = Ini::SplitInts(buf);
			if (rd.size() >= 2) { cfg.delayMin = rd[0]; cfg.delayMax = rd[1]; }
		}

		auto res = g_configs.emplace(pType, std::move(cfg));
		return res.first->second;
	}

	static int NextDelay(const HostConfig& cfg)
	{
		if (cfg.delayMax > cfg.delayMin && cfg.delayMax > 0)
			return ScenarioClass::Instance->Random.RandomRanged(cfg.delayMin, cfg.delayMax);
		return cfg.delay > 0 ? cfg.delay : 0;
	}

	void ForgetHost(TechnoClass* pTechno) { g_states.erase(pTechno); }

	void SaveHostState(IStream* stream)
	{
		unsigned count = static_cast<unsigned>(g_states.size());
		Serialize::Write(stream, count);
		for (auto& kv : g_states)
		{
			Serialize::WritePtr(stream, kv.first);
			Serialize::Write(stream, kv.second);
		}
	}

	void LoadHostState(IStream* stream)
	{
		g_states.clear();
		unsigned count = 0;
		if (!Serialize::Read(stream, count))
			return;
		for (unsigned i = 0; i < count; ++i)
		{
			void* p = Serialize::ReadSwizzled(stream);
			HostState st;
			if (!Serialize::Read(stream, st))
				return;
			if (p)
				g_states[static_cast<TechnoClass*>(p)] = st;
		}
	}

	void UpdateHost(TechnoClass* pTechno)
	{
		TechnoTypeClass* pType = pTechno->GetTechnoType();
		if (!pType)
			return;

		const HostConfig& cfg = GetHostConfig(pType);
		if (!cfg.enabled)
			return;

		// Chain-spawn guard: a unit produced by a Host/GiftBox never hosts copies.
		if (cfg.onlyBuilt && Spawn::IsGiftSpawned(pTechno))
			return;

		// Burst cap: stop after Host.TriggeredTimes bursts (0 = unlimited).
		HostState& st = g_states[pTechno];
		if (cfg.triggeredTimes > 0 && st.count >= cfg.triggeredTimes)
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

		HouseClass* pHouse = pTechno->Owner;
		CoordStruct origin = pTechno->GetCoords();
		auto gifts = Spawn::BuildGiftList(cfg.types, cfg.nums, cfg.chances, cfg.randomType, cfg.weights);
		Spawn::InheritSpec inherit;
		inherit.source = pTechno;
		inherit.health = cfg.inheritHealth || cfg.healthPercent > 0.0;
		inherit.healthPercent = cfg.healthPercent;
		inherit.veterancy = cfg.inheritVeterancy;
		int ok = Spawn::ReleaseList(gifts, pHouse, origin, cfg.randomRange, cfg.emptyCell, inherit);
		Log("[Host] %s burst by %p (cnt=%d): spawned %d/%d at (%d,%d,%d)",
			pType->ID, (void*)pTechno, st.count, ok, (int)gifts.size(),
			origin.X, origin.Y, origin.Z);

		++st.count;
		st.timer = NextDelay(cfg);
	}
}
