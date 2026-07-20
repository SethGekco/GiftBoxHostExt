// Host feature — standalone, direct-TechnoType-tag model.
//
// A TechnoType tagged with Host.Types spawns those units near itself on a timer
// while it is alive. Uses the game's synchronized RNG so it is netplay-safe.
// Host.OnlyBuilt prevents the chain-spawn problem: units that were themselves
// spawned by a Host do not spawn their own copies.
//
// INI (all keys on the spawning unit's TechnoType section, read from rulesmd.ini):
//   Host.Types=E1,E2      ; unit type IDs to spawn (required to enable)
//   Host.Nums=2,1         ; count per type, parallel to Types (default 1 each)
//   Host.Delay=300        ; frames between spawn bursts (default 0 = every frame)
//   Host.RandomDelay=250,350 ; optional; if set, each burst waits a synced-random delay in [min,max]
//   Host.InitialDelay=0   ; frames before the first burst
//   Host.OnlyBuilt=no     ; if yes, gift-spawned copies never spawn (chain guard)
#pragma once

#include <string>
#include <vector>

class TechnoClass;
class TechnoTypeClass;

namespace GiftBoxHost
{
	// Per-TechnoType configuration, parsed once from rulesmd.ini and cached.
	struct HostConfig
	{
		bool enabled = false;
		std::vector<std::string> types;   // Host.Types
		std::vector<int> nums;            // Host.Nums (parallel; default 1)
		int delay = 0;                    // Host.Delay
		int delayMin = 0;                 // Host.RandomDelay min (0 => use delay)
		int delayMax = 0;                 // Host.RandomDelay max
		int initialDelay = 0;             // Host.InitialDelay
		int triggeredTimes = 0;           // Host.TriggeredTimes (0 = unlimited)
		bool onlyBuilt = false;           // Host.OnlyBuilt
	};

	// Per-unit runtime state.
	struct HostState
	{
		bool initialized = false;   // has the timer been primed?
		int timer = 0;              // frames until next burst
		int count = 0;              // bursts performed (vs Host.TriggeredTimes)
		bool isGiftSpawned = false; // this unit was produced by a Host (chain guard)
	};

	// Returns the cached config for a type, parsing INI on first use.
	const HostConfig& GetHostConfig(TechnoTypeClass* pType);

	// Marks a freshly spawned unit as gift-spawned (sets its state flag).
	void MarkGiftSpawned(TechnoClass* pTechno);

	// Drops per-unit state (called from the TechnoClass destructor hook).
	void ForgetUnit(TechnoClass* pTechno);

	// Runs one tick of Host logic for a unit (called from the update hook).
	void UpdateHost(TechnoClass* pTechno);
}
