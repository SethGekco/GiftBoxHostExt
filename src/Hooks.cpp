// All Syringe hooks for the feature code live here; Host.cpp / GiftBox.cpp stay
// pure logic. These addresses are also hooked by Ares/Phobos — Syringe chains
// multiple hooks per address, and we keep no shared framework state, so this
// coexists cleanly (unlike two copies of a whole framework).

#include "Host.h"
#include "GiftBox.h"
#include "Spawn.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET
#include <FootClass.h>       // complete type needed by YRpp cast traits pulled via Macro.h
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

// BuildingClass::KickOutUnit entry -> mark the ejected unit "built", so OnlyBuilt
// admits only factory-produced units (paradrop/crate/map/spawned never reach here).
// Verified via objdump: 0x443C60 is the function entry (prev fn ends ret@0x443C5C,
// NOP pad @0x443C5F; the 0x443CCA aircraft branch is inside this fn). Signature
// KickOutUnit(TechnoClass* pTechno, CellStruct) -> pTechno is arg1 at [esp+4] on
// entry (the fn reloads it as `mov edi,[esp+0x144]` after `sub esp,0x130`+4 pushes).
// Size 0x6 = the first instruction `sub esp,0x130`. Unhooked address (frameworks
// hook the inner type branches at 0x443CCA/0x444119/0x444131, not the entry).
DEFINE_HOOK(0x443C60, GiftBoxHost_KickOutUnit_MarkBuilt, 0x6)
{
	GET_STACK(TechnoClass*, pTechno, 0x4);
	GiftBoxHost::Spawn::MarkBuilt(pTechno);
	return 0;
}
