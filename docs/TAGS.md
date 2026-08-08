# GiftBoxHost INI tags

All tags go **directly on the spawning unit's TechnoType** section in `rulesmd.ini`
(no `AttachEffect` wrapper — this is what lets GiftBoxHost coexist with Kratos).

## Host — spawn units on a unit while it lives

| Key | Type | Default | Meaning |
|-----|------|---------|---------|
| `Host.Types` | list of type IDs | *(none)* | Units to spawn. **Required** — presence enables Host. |
| `Host.Nums` | list of ints | `1` each | Count per entry in `Host.Types` (parallel list). |
| `Host.Delay` | int (frames) | `0` | Frames between spawn bursts. `0` = every frame. |
| `Host.RandomDelay` | `min,max` | *(off)* | If set, each burst waits a **synced-random** delay in `[min,max]`, overriding `Host.Delay`. |
| `Host.InitialDelay` | int (frames) | `0` | Delay before the first burst. |
| `Host.TriggeredTimes` | int | `0` | Max number of bursts per unit (`0` = unlimited). Use this to make a unit spawn only N times, then stop. |
| `Host.RandomRange` | int (cells) | `0` | Scatter radius. `0` = spawn on the host's own cell (units stack); higher = spread spawns into nearby cells. |
| `Host.RandomToEmptyCell` | bool | `yes` | When scattering, prefer cells the spawn can actually stand on (avoids stacking). Set `no` to allow occupied cells. |
| `Host.OnlyBuilt` | bool | `no` | If `yes`, units that were themselves spawned by a Host never spawn their own copies (prevents the chain-spawn explosion when `Host.Types` includes the host's own type). **Note:** this does *not* stop the originally-built unit from spawning on its timer — use `Host.TriggeredTimes` or `Host.Delay` to bound that. |

### Example

```ini
[GGI]                 ; Guardian GI hosts two conscripts every 300 frames
Host.Types=E1
Host.Nums=2
Host.Delay=300
Host.OnlyBuilt=yes    ; conscripts spawned this way won't spawn more

[SREF]                ; a unit that clones itself, safely bounded
Host.Types=SREF
Host.Nums=1
Host.Delay=450
Host.OnlyBuilt=yes    ; only the factory-built SREF clones; clones don't chain
```

## GiftBox — a "box" that releases units when it opens

A box opens either on a **timer** or, iconically, **when it's destroyed**
(`GiftBox.OpenWhenDestroyed=yes`), releasing its gifts. Shares Host's placement,
chain-guard and synced RNG.

| Key | Type | Default | Meaning |
|-----|------|---------|---------|
| `GiftBox.Types` | list of type IDs | *(none)* | Units to release. **Required** — enables GiftBox. |
| `GiftBox.Nums` | list of ints | `1` each | Count per entry in `GiftBox.Types`. |
| `GiftBox.OpenWhenDestroyed` | bool | `no` | If `yes`, the box opens **on death** (e.g. a transport that spills units when killed) instead of on a timer. |
| `GiftBox.Delay` | int (frames) | `0` | Timer mode: frames before the box opens. |
| `GiftBox.RandomDelay` | `min,max` | *(off)* | Timer mode: synced-random delay in `[min,max]`. |
| `GiftBox.InitialDelay` | int (frames) | `0` | Delay before the timer starts. |
| `GiftBox.Remove` | bool | `yes` | Timer mode: destroy the box after it opens (`no` = re-arm and open again). |
| `GiftBox.RandomRange` | int (cells) | `0` | Scatter radius for the released units. |
| `GiftBox.RandomToEmptyCell` | bool | `yes` | Prefer clear cells when scattering. |
| `GiftBox.OnlyBuilt` | bool | `no` | Gift-spawned boxes never open (chain guard). |

```ini
[TRANS]               ; transport that spills 3 GIs when destroyed
GiftBox.Types=E2
GiftBox.Nums=3
GiftBox.OpenWhenDestroyed=yes
GiftBox.RandomRange=2

[CRATE]               ; a timed box: opens after 5s, releases a dog, then vanishes
GiftBox.Types=DOG
GiftBox.Delay=75
GiftBox.Remove=yes
```

## Inheritance — spawned/released units carry over the source's state

These apply to **both** Host and GiftBox (prefix with `Host.` or `GiftBox.`). The
source is the spawning unit (the host) or the box.

| Key | Type | Default | Meaning |
|-----|------|---------|---------|
| `…InheritHealth` | bool | `no` | Give each spawned unit the **same health %** the source currently has. |
| `…HealthPercent` | float | `0` | Force a specific health fraction (`0.5` = 50 %). Overrides `InheritHealth`; `0` = off. |
| `…InheritVeterancy` | bool | `no` | Copy the source's veterancy (rookie/veteran/elite) to each spawned unit. |
| `GiftBox.InheritPassenger` | bool | `no` | **GiftBox only** — move the box's passengers into the released units (up to each unit's own passenger capacity). Meant for the "transport spills its cargo" case; not offered on Host since the host keeps living. |

```ini
[VETSPAWN]            ; an elite host spawns elite, full-health copies
Host.Types=E1
Host.Delay=200
Host.InheritVeterancy=yes
Host.InheritHealth=yes

[APOC]                ; a destroyed Apocalypse leaves a wounded veteran behind
GiftBox.Types=HTNK
GiftBox.OpenWhenDestroyed=yes
GiftBox.InheritVeterancy=yes
GiftBox.HealthPercent=0.4
```

## Notes

- All randomness uses the game's synchronized RNG, so Host and GiftBox are multiplayer-safe.
- Spawned units take the host/box owner (house).
- Host and GiftBox can be combined on the same unit if you want both behaviors.
- State (timers, caps, chain-guard flags) persists across savegames.
