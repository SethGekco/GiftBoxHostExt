// Persist our per-unit state across savegames. We append our data to the end of
// the savegame object stream and read it back at the symmetric load point,
// swizzling the stored TechnoClass* values to the relocated objects.
//
// A magic marker guards the load: if it isn't found (an older save, or another
// DLL changed the stream layout) we bail without touching anything, so old saves
// stay loadable and we never crash on unexpected data.

#include "Spawn.h"
#include "Host.h"
#include "GiftBox.h"

#include <Helpers/Macro.h>   // DEFINE_HOOK, GET
#include <FootClass.h>       // YRpp cast-trait completeness (see Hooks.cpp)
#include <windows.h>         // IStream

namespace
{
	constexpr unsigned GBH_SAVE_MAGIC = 0x31484247u; // "GBH1"
}

// End of writing the savegame object stream -> append our data. (ESI, size 0xD)
DEFINE_HOOK(0x67E42E, GiftBoxHost_SaveStream_End, 0xD)
{
	GET(IStream*, stream, ESI);
	ULONG n = 0;
	unsigned magic = GBH_SAVE_MAGIC;
	stream->Write(&magic, sizeof(magic), &n);
	GiftBoxHost::Spawn::SaveState(stream);
	GiftBoxHost::SaveHostState(stream);
	GiftBoxHost::SaveGiftBoxState(stream);
	return 0;
}

// End of reading the savegame object stream -> read our data (all objects exist
// now, so pointers can be swizzled). (ESI, size 0x5)
DEFINE_HOOK(0x67F7C8, GiftBoxHost_LoadStream_End, 0x5)
{
	GET(IStream*, stream, ESI);
	unsigned magic = 0;
	ULONG n = 0;
	if (stream->Read(&magic, sizeof(magic), &n) != S_OK || n != sizeof(magic) || magic != GBH_SAVE_MAGIC)
		return 0; // old/foreign save: leave everything as-is
	GiftBoxHost::Spawn::LoadState(stream);
	GiftBoxHost::LoadHostState(stream);
	GiftBoxHost::LoadGiftBoxState(stream);
	return 0;
}
