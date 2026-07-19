# GiftBoxHost

A **standalone** Yuri's Revenge [Syringe](https://github.com/Ares-Developers/Syringe) DLL that
implements the **Host** and **GiftBox** unit-spawning features — reimplemented cleanly, independent
of the Kratos framework, so it can coexist with other Syringe DLLs and avoids Kratos's shared-RNG
network-desync issue.

## Why this exists

Kratos-PP bundles these features into a large component framework whose shared pseudo-random
generator mixes **synchronized game-logic** randomness with **unsynchronized rendering** randomness
— a classic multiplayer-desync design (see [docs/DESIGN.md](docs/DESIGN.md)).

This project extracts just Host + GiftBox as a small, self-contained DLL that:

- Carries **none** of Kratos's framework (no component system, no `TechnoStatus`, no `AttachEffect`).
- Uses the game's **synchronized RNG** (`ScenarioClass::Instance->Random`) for all spawn logic → desync-safe by construction.
- Hooks only the minimal set of addresses it needs, so it can be loaded **alongside** other Syringe DLLs.
- Ports Kratos's actual spawn algorithm, so in-game behavior stays faithful.

## Status

Built in stages (each verified by CI on `windows-2022`):

- [x] **Stage 0** — minimal buildable skeleton (bootstrap + hook, no framework).
- [ ] **Stage 1** — lightweight per-unit state + INI parsing.
- [ ] **Stage 2** — Host (spawn-on-unit) with synced RNG + `Host.OnlyBuilt` chain-spawn guard.
- [ ] **Stage 3** — GiftBox.
- [ ] **Stage 4** — save/load, polish.

## Build

x86 MSVC (Syringe DLL). CI builds `GiftBoxHost.dll` via
`msbuild GiftBoxHost.sln /p:Configuration=Release /p:Platform=x86`.
YRpp is a submodule — clone with `--recursive`.

## Compatibility note

This DLL is **not** a trimmed Kratos; it is an independent reimplementation. It does not share INI
syntax with Kratos's `AttachEffect`-based Host — tags are placed **directly on the TechnoType**
(documented as features land). GiftBoxHost and Kratos use different code paths, so they do not
duplicate each other's framework work; but if you enable the *same* spawning ability in both, you
will get it twice. Enable each feature in exactly one place.

## License

LGPL v3 — see [LICENSE.md](LICENSE.md). Spawn algorithm derived from Kratos-PP (LGPL v3).
