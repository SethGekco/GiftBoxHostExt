// Host feature — standalone, direct-TechnoType-tag model.
//
// A TechnoType tagged with Host.Types spawns those units near itself on a timer
// while it is alive. Uses the game's synchronized RNG so it is netplay-safe.
// Host.OnlyBuilt prevents chain-spawning: units that were themselves spawned by
// a Host/GiftBox do not spawn their own copies.
//
// INI (all keys on the spawning unit's TechnoType section, read from rulesmd.ini):
//   Host.Types=E1,E2         ; unit type IDs to spawn (required to enable)
//   Host.Nums=2,1            ; count per type, parallel to Types (default 1 each)
//   Host.Delay=300           ; frames between spawn bursts (default 0 = every frame)
//   Host.RandomDelay=250,350 ; optional; synced-random delay in [min,max] per burst
//   Host.InitialDelay=0      ; frames before the first burst
//   Host.TriggeredTimes=0    ; max bursts per unit (0 = unlimited)
//   Host.RandomRange=0       ; scatter radius in cells (0 = host's own cell)
//   Host.RandomToEmptyCell=yes ; prefer cells the spawn can stand on
//   Host.OnlyBuilt=no        ; if yes, gift-spawned copies never spawn (chain guard)
#pragma once

#include <string>
#include <vector>

class TechnoClass;
class TechnoTypeClass;
struct IStream;

namespace GiftBoxHost
{
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
		int randomRange = 0;              // Host.RandomRange
		bool emptyCell = true;            // Host.RandomToEmptyCell
		bool onlyBuilt = false;           // Host.OnlyBuilt
		bool randomType = false;          // Host.RandomType
		std::vector<int> weights;         // Host.RandomWeights
		std::vector<double> chances;      // Host.Chances
	};

	struct HostState
	{
		bool initialized = false;   // has the timer been primed?
		int timer = 0;              // frames until next burst
		int count = 0;              // bursts performed (vs Host.TriggeredTimes)
	};

	const HostConfig& GetHostConfig(TechnoTypeClass* pType);

	// One tick of Host logic for a unit (called from the update hook).
	void UpdateHost(TechnoClass* pTechno);

	// Drop this unit's Host state (called from the TechnoClass destructor hook).
	void ForgetHost(TechnoClass* pTechno);

	void SaveHostState(IStream* stream);
	void LoadHostState(IStream* stream);
}
