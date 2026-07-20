#include "Host.h"
#include "Log.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET  (pulls in Syringe.h)

#include <GeneralDefinitions.h>   // DirType
#include <GeneralStructures.h>    // CoordStruct
#include <TechnoClass.h>
#include <TechnoTypeClass.h>
#include <HouseClass.h>
#include <MapClass.h>
#include <CellClass.h>
#include <ScenarioClass.h>
#include <CCINIClass.h>
#include <Unsorted.h>

#include <unordered_map>
#include <cstring>
#include <cctype>
#include <cstdlib>

namespace GiftBoxHost
{
	// ---- caches / registries -------------------------------------------------
	// Config is keyed by TechnoType (parsed once). State is keyed by live unit;
	// we only ever look up by the current unit pointer — never iterate for game
	// logic — so pointer keys are deterministic and netplay-safe.
	static std::unordered_map<TechnoTypeClass*, HostConfig> g_configs;
	static std::unordered_map<TechnoClass*, HostState> g_states;

	// ---- INI helpers ---------------------------------------------------------
	static bool IsNoneToken(const std::string& s)
	{
		return s.empty() || _stricmp(s.c_str(), "none") == 0 || _stricmp(s.c_str(), "<none>") == 0;
	}

	static std::string Trim(const std::string& s)
	{
		size_t a = 0, b = s.size();
		while (a < b && std::isspace((unsigned char)s[a])) ++a;
		while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
		return s.substr(a, b - a);
	}

	static std::vector<std::string> SplitList(const char* raw)
	{
		std::vector<std::string> out;
		std::string cur;
		for (const char* p = raw; ; ++p)
		{
			if (*p == ',' || *p == '\0')
			{
				std::string tok = Trim(cur);
				if (!IsNoneToken(tok)) out.push_back(tok);
				cur.clear();
				if (*p == '\0') break;
			}
			else
			{
				cur.push_back(*p);
			}
		}
		return out;
	}

	static std::vector<int> SplitInts(const char* raw)
	{
		std::vector<int> out;
		for (const std::string& tok : SplitList(raw))
		{
			out.push_back(atoi(tok.c_str()));
		}
		return out;
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
		cfg.types = SplitList(buf);
		if (!cfg.types.empty())
		{
			cfg.enabled = true;

			pINI->ReadString(section, "Host.Nums", "", buf, sizeof(buf));
			cfg.nums = SplitInts(buf);

			cfg.delay = pINI->ReadInteger(section, "Host.Delay", 0);
			cfg.initialDelay = pINI->ReadInteger(section, "Host.InitialDelay", 0);
			cfg.onlyBuilt = pINI->ReadBool(section, "Host.OnlyBuilt", false);

			pINI->ReadString(section, "Host.RandomDelay", "", buf, sizeof(buf));
			std::vector<int> rd = SplitInts(buf);
			if (rd.size() >= 2) { cfg.delayMin = rd[0]; cfg.delayMax = rd[1]; }
		}

		auto res = g_configs.emplace(pType, std::move(cfg));
		return res.first->second;
	}

	// ---- spawn primitive (ported from Kratos Gift.cpp, synced-RNG only) ------
	static bool TryPutTechno(TechnoClass* pTechno, CoordStruct location)
	{
		CellClass* pCell = MapClass::Instance->TryGetCellAt(location);
		if (!pCell)
			return false;

		pTechno->OnBridge = pCell->ContainsBridge();
		CoordStruct xyz = pCell->GetCoordsWithBridge();

		++Unsorted::IKnowWhatImDoing;
		pTechno->Unlimbo(xyz, DirType::East);
		--Unsorted::IKnowWhatImDoing;

		xyz.Z = location.Z;
		pTechno->SetLocation(xyz);
		return true;
	}

	static TechnoClass* CreateAndPutTechno(TechnoTypeClass* pType, HouseClass* pHouse, CoordStruct location)
	{
		// CreateObject for a TechnoType yields a TechnoClass-derived object
		// (single, non-virtual inheritance chain), so this downcast is valid.
		TechnoClass* pTechno = static_cast<TechnoClass*>(pType->CreateObject(pHouse));
		if (pTechno && TryPutTechno(pTechno, location))
			return pTechno;
		return nullptr;
	}

	// ---- timing --------------------------------------------------------------
	static int NextDelay(const HostConfig& cfg)
	{
		if (cfg.delayMax > cfg.delayMin && cfg.delayMax > 0)
			return ScenarioClass::Instance->Random.RandomRanged(cfg.delayMin, cfg.delayMax);
		return cfg.delay > 0 ? cfg.delay : 0;
	}

	// ---- public API ----------------------------------------------------------
	void MarkGiftSpawned(TechnoClass* pTechno)
	{
		g_states[pTechno].isGiftSpawned = true;
	}

	void ForgetUnit(TechnoClass* pTechno)
	{
		g_states.erase(pTechno);
	}

	void UpdateHost(TechnoClass* pTechno)
	{
		TechnoTypeClass* pType = pTechno->GetTechnoType();
		if (!pType)
			return;

		const HostConfig& cfg = GetHostConfig(pType);
		if (!cfg.enabled)
			return;

		HostState& st = g_states[pTechno];

		// Chain-spawn guard: a unit produced by a Host never hosts its own copies.
		if (cfg.onlyBuilt && st.isGiftSpawned)
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

		// Burst: spawn each configured type near the host.
		HouseClass* pHouse = pTechno->Owner;
		CoordStruct origin = pTechno->GetCoords();
		for (size_t i = 0; i < cfg.types.size(); ++i)
		{
			int count = (i < cfg.nums.size() && cfg.nums[i] > 0) ? cfg.nums[i] : 1;
			TechnoTypeClass* pSpawnType = TechnoTypeClass::Find(cfg.types[i].c_str());
			if (!pSpawnType)
				continue;
			for (int c = 0; c < count; ++c)
			{
				if (TechnoClass* pGift = CreateAndPutTechno(pSpawnType, pHouse, origin))
					MarkGiftSpawned(pGift);
			}
		}

		st.timer = NextDelay(cfg);
	}
}

// ---- hooks -------------------------------------------------------------------

// Per-unit update tick. thiscall -> ECX. Same address Kratos uses.
DEFINE_HOOK(0x6F9E50, GiftBoxHost_TechnoUpdate, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GiftBoxHost::UpdateHost(pThis);
	return 0;
}

// Techno destructor -> forget per-unit state so pointers can't go stale.
DEFINE_HOOK(0x6F4500, GiftBoxHost_TechnoDTOR, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GiftBoxHost::ForgetUnit(pThis);
	return 0;
}
