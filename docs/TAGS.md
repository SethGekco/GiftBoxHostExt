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
| `Host.OnlyBuilt` | bool | `no` | If `yes`, units that were themselves spawned by a Host never spawn their own copies (prevents the chain-spawn explosion when `Host.Types` includes the host's own type). |

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

## Notes

- All randomness uses the game's synchronized RNG, so Host is multiplayer-safe.
- Spawned units take the host's owner (house).
- v1 spawns at the host's own cell; positional scatter (`Host.RandomRange`, empty-cell
  search) will land in a later revision.
