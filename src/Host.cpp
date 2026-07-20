#include "Host.h"
#include "Spawn.h"
#include "Ini.h"
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
		for (size_t i = 0; i < cfg.types.size(); ++i)
		{
			int count = (i < cfg.nums.size() && cfg.nums[i] > 0) ? cfg.nums[i] : 1;
			TechnoTypeClass* pSpawnType = TechnoTypeClass::Find(cfg.types[i].c_str());
			if (!pSpawnType)
			{
				Log("[Host] %s: spawn type '%s' NOT FOUND", pType->ID, cfg.types[i].c_str());
				continue;
			}
			int ok = Spawn::Release(pSpawnType, pHouse, origin, count, cfg.randomRange, cfg.emptyCell);
			Log("[Host] %s burst by %p (cnt=%d): spawned %d/%d %s at (%d,%d,%d)",
				pType->ID, (void*)pTechno, st.count, ok, count, cfg.types[i].c_str(),
				origin.X, origin.Y, origin.Z);
		}

		++st.count;
		st.timer = NextDelay(cfg);
	}
}
