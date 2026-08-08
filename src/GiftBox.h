// GiftBox feature — standalone, direct-TechnoType-tag model.
//
// A "box" TechnoType that releases units ("gifts") when it opens. It opens either
// on a timer, or — the iconic case — when it is destroyed (GiftBox.OpenWhenDestroyed).
// Shares Host's spawn/placement/chain-guard machinery and the synced RNG.
//
// INI (keys on the box's TechnoType section, read from rulesmd.ini):
//   GiftBox.Types=E1,E2         ; unit type IDs to release (required to enable)
//   GiftBox.Nums=2,1            ; count per type (default 1 each)
//   GiftBox.Delay=0             ; timer mode: frames before it opens (default 0)
//   GiftBox.RandomDelay=a,b     ; optional synced-random delay in [a,b]
//   GiftBox.InitialDelay=0      ; delay before the timer starts
//   GiftBox.OpenWhenDestroyed=no; if yes, opens on death instead of on a timer
//   GiftBox.Remove=yes          ; timer mode: remove the box after it opens
//   GiftBox.Explodes=no         ; if removing, kill it with damage (explosion) vs silent
//   GiftBox.RandomRange=0       ; scatter radius in cells
//   GiftBox.RandomToEmptyCell=yes
//   GiftBox.OnlyBuilt=no        ; gift-spawned boxes never open (chain guard)
#pragma once

#include <string>
#include <vector>

class TechnoClass;
class TechnoTypeClass;
struct IStream;

namespace GiftBoxHost
{
	struct GiftBoxConfig
	{
		bool enabled = false;
		std::vector<std::string> types;
		std::vector<int> nums;
		int delay = 0;
		int delayMin = 0;
		int delayMax = 0;
		int initialDelay = 0;
		int randomRange = 0;
		bool emptyCell = true;
		bool onlyBuilt = false;
		bool openWhenDestroyed = false;
		bool remove = true;
		bool explodes = false;
		bool randomType = false;          // GiftBox.RandomType
		std::vector<int> weights;         // GiftBox.RandomWeights
		std::vector<double> chances;      // GiftBox.Chances
		bool inheritHealth = false;       // GiftBox.InheritHealth
		double healthPercent = 0.0;       // GiftBox.HealthPercent (0 = copy box %)
		bool inheritVeterancy = false;    // GiftBox.InheritVeterancy
		bool inheritPassenger = false;    // GiftBox.InheritPassenger
	};

	struct GiftBoxState
	{
		bool initialized = false;
		int timer = 0;
		bool opened = false;
	};

	const GiftBoxConfig& GetGiftBoxConfig(TechnoTypeClass* pType);

	// Timer-mode tick (from the update hook).
	void UpdateGiftBox(TechnoClass* pTechno);

	// Death trigger (from the ReceiveDamage-destroy hook, 0x702050).
	void GiftBoxOnDestroyed(TechnoClass* pTechno);

	// Drop this unit's GiftBox state (from the TechnoClass destructor hook).
	void ForgetGiftBox(TechnoClass* pTechno);

	void SaveGiftBoxState(IStream* stream);
	void LoadGiftBoxState(IStream* stream);
}
