# Design notes

## The Kratos multiplayer-desync hypothesis

While extracting Host/GiftBox from Kratos-PP we found what looks like the (or a) root cause of
Kratos's online instability. It is documented here both to justify this project's design and to
share with the Kratos team.

### The flaw

Kratos routes most of its randomness through one helper:

```cpp
// src/Ext/Helper/MathEx.h (Kratos)
class Random {
    static int    RandomRanged(int min, int max);   // draws from _engine
    static float  RandomFloat(float, float);        // draws from _engine
    static double RandomDouble();                    // draws from _engine
    inline static std::minstd_rand _engine{};        // ONE global generator
};
```

`_engine` is seeded **once per scenario** from the synchronized `Game::Seed`
(`GeneralHook.cpp`, hook `0x6875F3`, comment *"ensure network synchronization"*). That part is
correct — every client starts the generator in the same state.

The problem is that this **single** generator is then drawn from by ~40 call sites across ~23 files
that mix two fundamentally different kinds of randomness:

- **Synchronized game-logic** draws — e.g. Host/GiftBox spawn counts, positions, chances.
- **Unsynchronized presentation** draws — e.g. `DrawEx.cpp::DrawLaser` (random laser RGB and
  "VisualScatter"), `DamageText.cpp` (floating-combat-text X/Y offsets), trails, and other
  render-time effects.

A shared PRNG only stays in lockstep across clients if **every client draws from it the same number
of times, in the same order.** Rendering does not satisfy that: how many lasers/among damage texts a
given client draws depends on camera position, what is on-screen, and framerate. Each such visual
draw advances the shared engine, so the engine state **diverges** between clients, and every
subsequent **game-logic** draw returns different values on different machines → desync.

For contrast, the game's true synchronized RNG is `ScenarioClass::Instance->Random`, which Kratos
uses correctly in only a few places (e.g. `GeneralUtils.cpp`).

### Suggested fix for Kratos

Separate the two concerns: use a synchronized generator (seeded from `Game::Seed`, or just
`ScenarioClass::Instance->Random`) that is drawn from **only** in game-logic paths, and a completely
separate generator for presentation/visual randomness. Never share one engine between them.

### How GiftBoxHost avoids it

This DLL contains no visual RNG consumers and routes all spawn logic through
`ScenarioClass::Instance->Random`, so it is desync-safe by construction. Removing the visual
consumers is also *why* isolating Host/GiftBox into their own DLL can behave correctly online where
full Kratos does not.

*Status: strong static-analysis hypothesis. Final confirmation requires a real two-client online
test.*
