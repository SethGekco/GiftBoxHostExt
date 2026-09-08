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

## Host grants — a factory stamps Host behaviour onto the units it produces

These go on a **producing building** (barracks, war factory, cloning vats, etc).
Every unit that *exits that factory* (`KickOutUnit`) has the grant applied on top
of whatever Host tags its own TechnoType already carries — granted jobs run
**in addition to** the unit's own Host, each as an independent job with its own
type / amount / delay / fire-count. Grants never chain: a unit spawned by a host
never passes through `KickOutUnit`, so it never receives a grant.

| Key | Type | Default | Meaning |
|-----|------|---------|---------|
| `Host.AddTypes` | list of type IDs | *(none)* | Types the granted host spawns. Presence enables a grant. |
| `Host.AddAmount` | list of ints | `1` each | Per-burst count, parallel to `Host.AddTypes` (a single value applies to all types). |
| `Host.AddDelay` | list of ints (frames) | `0` each | Frames between bursts, **per type**. `0` = every frame. |
| `Host.AddCount` | list of ints | `0` each | Max bursts **per type** (`0` = unlimited). |
| `Host.RatioAmount` | float | `1.0` | Multiplies **every** host count on units this factory builds — their own Host *and* the granted jobs. `0.0` disables hosting for that unit (single forever). |
| `Host.RoundUp` | bool | `no` | When `RatioAmount` gives a fraction, round up (`yes`) or down (`no`). |

**Indexed grants.** For several independent grants on one factory, index them:
`Host.AddTypes[1]=`, `Host.AddAmount[1]=`, `Host.AddDelay[1]=`, `Host.AddCount[1]=`,
and so on (the unbracketed form is index 0; `[0]` is accepted as a synonym).

### Example

```ini
[GAPILE]              ; every unit this barracks builds also hosts, on top of its own tags
Host.AddTypes=E1,E2   ; E1 and E2 are granted as two independent jobs
Host.AddAmount=3,1    ; E1 x3 per burst, E2 x1
Host.AddDelay=0,200   ; E1 immediately, E2 every 200 frames
Host.AddCount=1,4     ; E1 fires once, E2 fires 4 times

[NAWEAP]              ; a war factory whose products host twice as much, rounding up
Host.RatioAmount=2.0
Host.RoundUp=yes

[GACNST]              ; a construction yard that suppresses hosting on everything it builds
Host.RatioAmount=0.0
```

> **Not yet included (next phase):** per-grant `Prerequisite` / `RequiredHouses` /
> power gating. Granted jobs also do not yet persist across a save/load if the unit
> has *no* own Host tags (its own Host and the `RatioAmount` do persist).

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
