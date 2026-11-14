# Touchdown Mars: First Light · Level Design Document (level-design.md)

> **Canonical design source:** `gdd.md` (Level Design, Themes, Game Flow). This document turns the GDD levels into buildable detail. **Where this document and the GDD disagree, the GDD wins.**
> **Companion documents:** `gdd.md` and `technical_design.md`.

**World overview:** `L_TD_Test` (greybox test map) → `L_TD_Outpost` (Mars, the only required level) → `L_TD_MoonOutpost` (optional, cut first). The game is one fixed view; "levels" of play are the six mission phases inside `L_TD_Outpost`.

---

## 1. Level Design Philosophy
1. **Guidance:** the muted, still world never competes with the three systems. Each system has its own colored light that brightens with its level. The next valid move pulses (cell glow, ghost hand after 6-8 s idle). Sound confirms every click.
2. **Teaching structure:** introduce (phase 1: one cell to AIR, ghost hand) → combine (phase 2: cells across systems plus a power-hookup failure) → twist (phase 3: solar storm, shelter vs science) → final test (phase 5: blackout with no advice), then the goal (phase 6: lift-off).
3. **Layout style:** single diorama seen from one fixed camera; every important object inside the top two thirds of the screen.

**Scale standard:** crew height 180 cm in suits; habitat door 220 cm; habitat module about 8 m long; greenhouse dome about 6 m wide; shelter about 4 m; fission unit placed about 60 m behind the habitat (visually small, with energy lines running to the base); lander about 40 m back. MISSING: exact fission-unit distance from a NASA source; until then the distance is `[DESIGNER VALUE]`.

---

## 2. Mechanic Vocabulary
> Every phase is built ONLY from these verbs.

| Verb / mechanic | Who uses it | GDD source | Level use |
| :-- | :-- | :-- | :-- |
| Place / take a power cell | Child | §2 Mechanic 1 | Every phase |
| Answer an event card (A or B) | Child | §2 Mechanic 2 | Phases 2-5 |
| Flip and read "What really happened?" | Child | §1 Main Mechanics 2 | Phases 2-5 |
| Call home | Child | §2 Mechanic 3 | Every phase except 5 on Mars (button disabled, "No signal") |
| Warn, Gather, Work | Child | §1 Main Mechanics 4 | Radiation cards |
| End phase | Child | §2 Controls | Every phase |
| System reacts (light, fan, plants, door) | Scene | §1 Gameplay | Every phase |
| Crew behaviour (work, worry, alarm, shelter, lander, celebrate) | Crew | §4 AI | Every phase |
| Rover drives a short path | Scene | §3 Game Flow | Phase 3 only |
| Lift-off | Scene | §3 Game Flow | Phase 6 end |

---

## 3. LEVEL 1: Mars Outpost (`L_TD_Outpost`)

**Theme / mood:** realistic Mars at the outpost site, late afternoon light, butterscotch sky; dark and quiet at the opening; dust haze in the dust-storm phase; warm system lights against cool shadows.
**Threats:** event cards only: power hookup failure, solar storm and radiation, dust storm (auxiliary solar array dims; fission keeps running), equipment failure, communication blackout.
**Goal:** reach lift-off at the end of phase 6 with the crew safe and the base working.

**Site:** LOCKED 13.11.2026 (team kept the proposal): Jezero crater floor near the Perseverance landing area, terrain shape from NASA Mars Trek elevation data (MISSING: exact Mars Trek product name, resolution and URL; Role C downloads it on 14 Nov and records it in `sources.csv`).

### Layout (14.11.2026: connected base, `Scripts/setup_base.py`)
Hub-and-spoke base, about 70 m x 60 m plus the power yard 60 m out. All positions are `[DESIGNER VALUE]` layout choices.
```
                         [LZ-2 cargo pad + logistics crates]
        [LZ-1 ascent pad + ascent vehicle + RASSOR]
                 \ graded road
   [Spares]--[B4 AIRLOCK]--(rover SEV parked)        [Solar fields x3]   [POWER YARD: fission units 1-4]
                 |                                         ^                     |
   [Greenhouse 2]-[Greenhouse]==[B1 COMMAND hub]==[B2 LIFE SUPPORT]==pipe==[ISRU plant + MOXIE]
                                    ||    (dish)            ||          ^ cable trays from the power yard
                              [Shelter + water wall      [B3 SCIENCE LAB]   [UHF relay antenna]
                               or regolith berm]
   player start (-X) ---> faces the hub
```
- **Corridors:** pressurised tubes, 2 m across, ribs every 3 m on supports (hub-greenhouse, hub-shelter, hub-life support, life support-lab, life support-airlock, greenhouse-greenhouse 2).
- **Design-driven parts** (tag `DesignShow_<OptionId>_<N>`, shown when the flown design has at least N): fission units 1-4, solar fields 1-3, water wall, regolith berm, second greenhouse, UHF relay antenna, MEDA mast, MOXIE unit, spare-part stacks. The scene behind the design room updates with every pick.
- **Labels:** B1 COMMAND, B2 LIFE SUPPORT, B3 SCIENCE LAB, B4 AIRLOCK, ISRU PLANT, LZ-1 ASCENT, LZ-2 CARGO, POWER YARD. The helmet names each part (Inspect_* keys in text.csv).
- **Rocks** inside the base footprint are hidden (tag `HiddenByBase`). The diorama's floating energy lines are off (`ATDPowerUnit::bShowEnergyLines`); cable trays carry the power.
- **Rebuild order:** setup_cameras → setup_props → setup_tripo → setup_base → setup_ops → setup_fps. `setup_ops.py` adds OpsShow_<TaskId> tags (rover, weather mast) and the deployed parts (payload crates at LZ-2, regolith mound on the shelter, survey stakes at the clay site).

### Progression
- **1. Introduce:** phase 1 Arrival (sol 1-3). Opening minute: dark base, "You are Stay", one cell to AIR with the ghost hand, three systems named, second cell with two valid slots, Call home, first card question. Nearly impossible to lose.
- **2. Combine:** phase 2 First walk (sol 4-5) and phase 3 Exploration (sol 7-10). All cells across systems; power hookup failure (fix now or lose power later); solar-storm warning (shelter now or keep working for a bonus sample); optional ice search or weather station.
- **3. Twist:** phase 4 Resupply and rest (sol 11-13): equipment failure, repair with what you have; optional plant experiment. Phase 5 Hard week: communication blackout, no advice, reduced margin.
- **4. Final test:** phase 6 Return (sol 28-31): load samples, secure the base for the next crew, lift off; Mission Report.

### Secrets & extras
- Optional science tasks (ice search, weather station, plant experiment, robot science left running): each costs cells and adds a Bonus line.
- Lettuce harvest at phase 6 if GREENHOUSE is Good; peppers shown as "won't grow this mission".
- Mission patch named after play style, not announced in advance.
- Seed shown on the report so a class can fly the same mission.

### Enemy / hazard notes
- No enemies. Hazards are cards drawn by the seeded deck; each game guarantees one Power, one Radiation and one Comms event.
- Upcoming-event icon appears over the system the next event will hit, with a countdown mark.
- Dust haze (`NS_TD_DustHaze`) plays only during the dust-storm card phase; radiation shows as a slow light sweep on the shelter, never as glowing crew.

---

## 4. LEVEL 2: Moon Outpost (`L_TD_MoonOutpost`, optional)

**Theme / mood:** black sky, hard white sun, grey regolith, long black shadows.
**Threats:** same card families; phase 5 is the lunar night instead of a blackout.
**Goal:** same as Mars.

### Layout
```
Same layout as L_TD_Outpost; Moon terrain and lighting; Earth visible in the sky.
```

### Progression
- **1. Introduce:** same opening; CAPCOM answers almost at once (1.3 s light time).
- **2. Combine:** same as Mars phases 2-3 with lunar cards (August 1972 storm, labelled "Model estimate").
- **3. Twist:** lunar night: power demand rises.
- **4. Final test:** return and lift-off.

### Secrets & extras
- The fast replies show the Mars delay by contrast.

### Enemy / hazard notes
- Uses Moon surface dose 0.81 mSv/day (George et al. 2024).

---

## 5. Difficulty Curve
| Phase (level) | New mechanic | Difficulty (1 to 5) | Notes |
| :-- | :-- | :-- | :-- |
| 1 Arrival | Allocation, Call home | 1 | Tutorial; demand 1 per system |
| 2 First walk | Event cards, card flip | 2 | Power hookup failure |
| 3 Exploration | Radiation, Warn-Gather-Work, optional science | 3 | First real trade-off: safety vs bonus |
| 4 Resupply and rest | Delayed greenhouse result | 3 | Equipment failure |
| 5 Hard week | No advice (blackout) | 4 | Reduced margin |
| 6 Return | Secure base, report | 2 | Payoff and pride |
| Moon mode | Contrast of delay | 2 | Optional |

---

## 6. Build Notes
1. **Greybox in `L_TD_Test`** (pipeline phases 1-3): cubes for the three systems, a plane for the ground, camera rig, debug text.
2. **Greybox in `L_TD_Outpost`** (phase 4): flat landscape, block meshes at the final positions, energy lines, crew as capsules, camera fixed. Test the opening minute here.
3. **Mechanics complete** (phases 5-8) before any art.
4. **Art pass** (phase 10): import Mars Trek elevation as landscape; Megascans surface materials via VibeUE `fab`; replace blocks with NASA and Tripo models; lights; dust and storm VFX.
5. **Audio pass** (phase 11).
6. **Moon level** (phase 12) only if Package 3 is green and before freeze.
