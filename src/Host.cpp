#include "Host.h"
#include "Spawn.h"
#include "Ini.h"
#include "Serialize.h"
#include "Log.h"

#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <UnitTypeClass.h>
#include <InfantryTypeClass.h>
#include <AircraftTypeClass.h>
#include <BuildingTypeClass.h>
#include <HouseClass.h>
#include <ScenarioClass.h>
#include <CCINIClass.h>

#include <unordered_map>
#include <cmath>
#include <cstdio>

namespace GiftBoxHost
{
	static std::unordered_map<TechnoTypeClass*, HostConfig> g_configs;
	static std::unordered_map<TechnoTypeClass*, BuildingGrantConfig> g_grants;
	static std::unordered_map<TechnoClass*, HostState> g_states;

	// Resolve a type ID across all four TechnoType arrays (units/infantry first —
	// that is what host grants target).
	static TechnoTypeClass* FindTechnoType(const char* id)
	{
		if (auto p = UnitTypeClass::Find(id))     return p;
		if (auto p = InfantryTypeClass::Find(id)) return p;
		if (auto p = AircraftTypeClass::Find(id)) return p;
		if (auto p = BuildingTypeClass::Find(id)) return p;
		return nullptr;
	}

	// Host.RatioAmount applied to a count. 0.0 -> 0 (hosting disabled). RoundUp
	// picks ceil vs floor for fractional results.
	static int ApplyRatio(int n, double ratio, bool roundUp)
	{
		if (ratio == 1.0)
			return n;
		double v = n * ratio;
		if (v <= 0.0)
			return 0;
		return roundUp ? static_cast<int>(std::ceil(v)) : static_cast<int>(std::floor(v));
	}

	// Broadcast rule for a parallel int list: a single value applies to every
	// type; a full list indexes per-type; anything shorter falls back to def.
	static int PickInt(const std::vector<int>& v, size_t i, int def)
	{
		if (v.empty())    return def;
		if (v.size() == 1) return v[0];
		return (i < v.size()) ? v[i] : def;
	}

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

	const BuildingGrantConfig& GetBuildingGrants(TechnoTypeClass* pType)
	{
		auto it = g_grants.find(pType);
		if (it != g_grants.end())
			return it->second;

		BuildingGrantConfig g;
		CCINIClass* pINI = CCINIClass::INI_Rules;
		const char* section = pType->ID;
		char buf[1024];

		g.ratio = pINI->ReadDouble(section, "Host.RatioAmount", 1.0);
		g.roundUp = pINI->ReadBool(section, "Host.RoundUp", false);

		// Read one indexed grant entry from the given key names; skip if no types.
		auto tryEntry = [&](const char* tk, const char* ak, const char* dk, const char* ck)
		{
			pINI->ReadString(section, tk, "", buf, sizeof(buf));
			std::vector<std::string> types = Ini::SplitList(buf);
			if (types.empty())
				return;

			GrantEntry e;
			e.types = std::move(types);
			pINI->ReadString(section, ak, "", buf, sizeof(buf)); e.amounts = Ini::SplitInts(buf);
			pINI->ReadString(section, dk, "", buf, sizeof(buf)); e.delays  = Ini::SplitInts(buf);
			pINI->ReadString(section, ck, "", buf, sizeof(buf)); e.counts  = Ini::SplitInts(buf);
			g.entries.push_back(std::move(e));
		};

		// Index 0 is the unbracketed form; [0] is accepted as a synonym.
		tryEntry("Host.AddTypes", "Host.AddAmount", "Host.AddDelay", "Host.AddCount");
		tryEntry("Host.AddTypes[0]", "Host.AddAmount[0]", "Host.AddDelay[0]", "Host.AddCount[0]");

		const int MAX_GRANT_ENTRIES = 32;
		for (int idx = 1; idx <= MAX_GRANT_ENTRIES; ++idx)
		{
			char tk[48], ak[48], dk[48], ck[48];
			std::snprintf(tk, sizeof(tk), "Host.AddTypes[%d]", idx);
			std::snprintf(ak, sizeof(ak), "Host.AddAmount[%d]", idx);
			std::snprintf(dk, sizeof(dk), "Host.AddDelay[%d]", idx);
			std::snprintf(ck, sizeof(ck), "Host.AddCount[%d]", idx);
			tryEntry(tk, ak, dk, ck);
		}

		g.enabled = !g.entries.empty() || g.ratio != 1.0;

		auto res = g_grants.emplace(pType, std::move(g));
		return res.first->second;
	}

	void ApplyBuildingGrants(TechnoClass* pBuilding, TechnoClass* pUnit)
	{
		if (!pBuilding || !pUnit)
			return;

		TechnoTypeClass* pbt = pBuilding->GetTechnoType();
		if (!pbt)
			return;

		const BuildingGrantConfig& g = GetBuildingGrants(pbt);
		if (!g.enabled)
			return;

		// RatioAmount scales EVERY host count on this unit (its own Host and any
		// granted jobs), baked in permanently for this unit.
		HostState& st = g_states[pUnit];
		st.ratio = g.ratio;
		st.roundUp = g.roundUp;

		for (const auto& e : g.entries)
		{
			for (size_t i = 0; i < e.types.size(); ++i)
			{
				TechnoTypeClass* pt = FindTechnoType(e.types[i].c_str());
				if (!pt)
				{
					Log("[Host] grant on %s: unknown type '%s'", pbt->ID, e.types[i].c_str());
					continue;
				}
				GrantJob job;
				job.type   = pt;
				job.amount = PickInt(e.amounts, i, 1);
				job.delay  = PickInt(e.delays,  i, 0);
				job.count  = PickInt(e.counts,  i, 0);
				st.grantJobs.push_back(job);
			}
		}

		Log("[Host] %s granted %u job(s) + ratio %.3f to %p",
			pbt->ID, (unsigned)st.grantJobs.size(), st.ratio, (void*)pUnit);
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
		// HostState now holds a std::vector (grantJobs), so it can no longer be
		// written by a raw struct copy — the POD fields are serialized one by one.
		unsigned count = static_cast<unsigned>(g_states.size());
		Serialize::Write(stream, count);
		for (auto& kv : g_states)
		{
			Serialize::WritePtr(stream, kv.first);
			const HostState& st = kv.second;
			Serialize::Write(stream, st.initialized);
			Serialize::Write(stream, st.timer);
			Serialize::Write(stream, st.count);
			Serialize::Write(stream, st.ratio);
			Serialize::Write(stream, st.roundUp);

			// Granted jobs carry a TechnoType pointer; persist it swizzled so it
			// resolves against the reloaded type array.
			unsigned njobs = static_cast<unsigned>(st.grantJobs.size());
			Serialize::Write(stream, njobs);
			for (const auto& j : st.grantJobs)
			{
				Serialize::WritePtr(stream, j.type);
				Serialize::Write(stream, j.amount);
				Serialize::Write(stream, j.delay);
				Serialize::Write(stream, j.count);
				Serialize::Write(stream, j.timer);
				Serialize::Write(stream, j.fired);
			}
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
			if (!Serialize::Read(stream, st.initialized)) return;
			if (!Serialize::Read(stream, st.timer))       return;
			if (!Serialize::Read(stream, st.count))       return;
			if (!Serialize::Read(stream, st.ratio))       return;
			if (!Serialize::Read(stream, st.roundUp))     return;

			unsigned njobs = 0;
			if (!Serialize::Read(stream, njobs)) return;
			for (unsigned j = 0; j < njobs; ++j)
			{
				GrantJob job;
				job.type = static_cast<TechnoTypeClass*>(Serialize::ReadSwizzled(stream));
				if (!Serialize::Read(stream, job.amount)) return;
				if (!Serialize::Read(stream, job.delay))  return;
				if (!Serialize::Read(stream, job.count))  return;
				if (!Serialize::Read(stream, job.timer))  return;
				if (!Serialize::Read(stream, job.fired))  return;
				st.grantJobs.push_back(job);
			}

			if (p)
				g_states[static_cast<TechnoClass*>(p)] = std::move(st);
		}
	}

	// Run the factory-granted host jobs on a unit. Each granted TYPE is an
	// independent job with its own delay/count/timer. These exist only on units a
	// factory kicked out, so they are inherently chain-safe (a spawned copy never
	// passes through KickOutUnit and so never carries grant jobs).
	static void RunGrantJobs(TechnoClass* pTechno, HostState& st, const HostConfig& cfg)
	{
		if (st.grantJobs.empty())
			return;

		HouseClass* pHouse = pTechno->Owner;
		// Scatter granted spawns at least one cell so they do not stack; honour a
		// wider Host.RandomRange if the unit's own Host set one.
		int range = cfg.randomRange > 0 ? cfg.randomRange : 1;

		for (auto& job : st.grantJobs)
		{
			if (!job.type)
				continue;
			if (job.count > 0 && job.fired >= job.count)
				continue;

			if (job.timer < 0)               // prime on first tick
				job.timer = job.delay;
			if (job.timer > 0)
			{
				--job.timer;
				continue;
			}

			int amt = ApplyRatio(job.amount, st.ratio, st.roundUp);
			if (amt > 0)
			{
				std::vector<TechnoTypeClass*> list(static_cast<size_t>(amt), job.type);
				Spawn::InheritSpec none;
				int ok = Spawn::ReleaseList(list, pHouse, pTechno, range, none);
				Log("[Host] grant burst by %p: %s x%d (fire %d/%d) -> %d placed",
					(void*)pTechno, job.type->ID, amt, job.fired + 1, job.count, ok);
			}

			++job.fired;
			job.timer = job.delay;
		}
	}

	void UpdateHost(TechnoClass* pTechno)
	{
		TechnoTypeClass* pType = pTechno->GetTechnoType();
		if (!pType)
			return;

		const HostConfig& cfg = GetHostConfig(pType);

		// A unit needs servicing here if it hosts on its own (cfg.enabled) OR it
		// carries factory-granted state (an entry already exists). Avoid inserting
		// a state entry for every techno in the game.
		auto it = g_states.find(pTechno);
		bool hasState = (it != g_states.end());
		if (!cfg.enabled && !hasState)
			return;

		HostState& st = hasState ? it->second : g_states[pTechno];

		// Granted jobs run regardless of OnlyBuilt — they only ever exist on units
		// a factory kicked out, so they cannot drive a spawn chain.
		RunGrantJobs(pTechno, st, cfg);

		// From here on: the unit's OWN Host tags.
		if (!cfg.enabled)
			return;

		// OnlyBuilt: only factory-built units Host. Excludes spawned copies (chain
		// guard) AND paradropped / crate / map-placed units — none pass KickOutUnit.
		if (cfg.onlyBuilt && !Spawn::IsBuilt(pTechno))
			return;

		// RatioAmount 0.0 disables the unit's own Host entirely (single forever).
		if (st.ratio == 0.0)
			return;

		// Burst cap: stop after Host.TriggeredTimes bursts (0 = unlimited).
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

		// Apply RatioAmount to the per-type counts (default ratio 1.0 = unchanged).
		const std::vector<int>* pNums = &cfg.nums;
		std::vector<int> scaled;
		if (st.ratio != 1.0)
		{
			scaled.reserve(cfg.types.size());
			for (size_t i = 0; i < cfg.types.size(); ++i)
			{
				int base = (i < cfg.nums.size()) ? cfg.nums[i] : 1;
				scaled.push_back(ApplyRatio(base, st.ratio, st.roundUp));
			}
			pNums = &scaled;
		}
		auto gifts = Spawn::BuildGiftList(cfg.types, *pNums, cfg.chances, cfg.randomType, cfg.weights);
		Spawn::InheritSpec inherit;
		inherit.source = pTechno;
		inherit.health = cfg.inheritHealth || cfg.healthPercent > 0.0;
		inherit.healthPercent = cfg.healthPercent;
		inherit.veterancy = cfg.inheritVeterancy;
		int ok = Spawn::ReleaseList(gifts, pHouse, pTechno, cfg.randomRange, inherit);
		Log("[Host] %s burst by %p (cnt=%d): spawned %d/%d at (%d,%d,%d)",
			pType->ID, (void*)pTechno, st.count, ok, (int)gifts.size(),
			origin.X, origin.Y, origin.Z);

		++st.count;
		st.timer = NextDelay(cfg);
	}
}
