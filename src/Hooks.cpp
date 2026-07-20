// All Syringe hooks for the feature code live here; Host.cpp / GiftBox.cpp stay
// pure logic. These addresses are also hooked by Ares/Phobos — Syringe chains
// multiple hooks per address, and we keep no shared framework state, so this
// coexists cleanly (unlike two copies of a whole framework).

#include "Host.h"
#include "GiftBox.h"
#include "Spawn.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET
#include <TechnoClass.h>

// Per-unit update tick (thiscall -> ECX).
DEFINE_HOOK(0x6F9E50, GiftBoxHost_TechnoUpdate, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GiftBoxHost::UpdateHost(pThis);
	GiftBoxHost::UpdateGiftBox(pThis);
	return 0;
}

// Destroyed by damage -> GiftBox.OpenWhenDestroyed (this -> ESI).
DEFINE_HOOK(0x702050, GiftBoxHost_TechnoDestroyed, 0x6)
{
	GET(TechnoClass*, pThis, ESI);
	GiftBoxHost::GiftBoxOnDestroyed(pThis);
	return 0;
}

// Techno destructor -> forget per-unit state so pointers can't go stale (-> ECX).
DEFINE_HOOK(0x6F4500, GiftBoxHost_TechnoDTOR, 0x5)
{
	GET(TechnoClass*, pThis, ECX);
	GiftBoxHost::ForgetHost(pThis);
	GiftBoxHost::ForgetGiftBox(pThis);
	GiftBoxHost::Spawn::Forget(pThis);
	return 0;
}
