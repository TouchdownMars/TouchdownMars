# Touchdown Mars: First Light · Model Card

**Which numbers are NASA data and which are game design**

Touchdown Mars is a game, not a simulator. Some numbers come from NASA documents, NASA/JPL services or published measurements; the rest are game rules chosen by Team Touchdown so that children aged 7-12 can play a 31-sol mission in a few minutes. This card lists both. The same split is shown in the game's Source Panel ("Where does this number come from?"), where every designer value carries the line "Game value chosen by the team, not a NASA measurement."

- Data files (source of truth): `Data/Source/constants.csv`, `sources.csv`, `events.csv`, `phases.csv`, `ageband.csv`, `text.csv`.
- All sources were opened and checked on 2026-11-13 (the `RetrievedOn` date in `sources.csv`). Card facts: `docs/cards_check.md`.
- Stored values are never shown as live. Only the Today's Sun panel can say `LIVE`, and only for DONKI data fetched during the current session, with the fetch time. Everything else is `ARCHIVE` with its date.

Not endorsed by NASA.

---

## 1. Constants (`constants.csv`)

### 1.1 Values from NASA and published sources

| Key | Value | Unit | NASA data or designer value | Source (detail) | Retrieved |
| :-- | --: | :-- | :-- | :-- | :-- |
| `O2PerPersonDay` | 0.895 | kg per crew member per day | NASA data (baseline value) | BVAD2: NASA Life Support BVAD Rev 2, Table 3-31 (PDF p. 72), oxygen consumed, 82 kg crew member | 2026-11-13 |
| `MarsSurfaceDose` | 0.65 | mSv per day | Published measurement | George2024: George et al. 2024, Nature, Table 2, average dose equivalent rate, Mars surface | 2026-11-13 |
| `MoonSurfaceDose` | 0.81 | mSv per day | Published measurement | George2024: Table 2, lunar surface | 2026-11-13 |
| `MarsTransitDose` | 1.58 | mSv per day | Published measurement | George2024: Table 2, Mars transit | 2026-11-13 |
| `CareerLimit` | 600 | mSv | NASA standard (limit) | OCHMO: career effective dose limit, NASA-STD-3001 Vol. 1 Rev B (brief p. 4) | 2026-11-13 |
| `LightTimeNow` | 11.014 | minutes | NASA/JPL ephemeris (computed) | HorizonsMars: one-way light time Earth-Mars, 2026-11-14 00:00 UT | 2026-11-13 |
| `LightTimeMin` | 3.2 | minutes | NASA/JPL ephemeris (computed) | HorizonsSeries: minimum of daily series 2017-2032, on 2018-07-31 | 2026-11-13 |
| `LightTimeMax` | 22.25 | minutes | NASA/JPL ephemeris (computed) | HorizonsSeries: maximum of daily series 2017-2032, on 2019-08-29 | 2026-11-13 |
| `MoonLightTime` | 1.353 | seconds | NASA/JPL ephemeris (computed) | HorizonsMoon: one-way light time Earth-Moon, 2026-11-14 00:00 UT (0.02255 min) | 2026-11-13 |
| `LettuceDays` | 33 | days | NASA report | Veggie: VEG-01 red romaine lettuce grew for 33 days (fact sheet p. 3) | 2026-11-13 |
| `PepperDays` | 137 | days | NASA report | Pepper2021: Plant Habitat-04 chile peppers | 2026-11-13 |

`LightTimeNow` is a stored value for one date (14 Nov 2026). It is not recomputed when the game runs.

### 1.2 Designer values (game rules, not NASA measurements)

| Key | Value | Unit | NASA data or designer value | Where it is defined | Retrieved |
| :-- | --: | :-- | :-- | :-- | :-- |
| `StartLevelAir` | 3 | level | Designer value | gdd.md §2 Mechanic 1 | n/a |
| `StartLevelGreenhouse` | 2 | level | Designer value | gdd.md §2 Mechanic 1 | n/a |
| `StartLevelShelter` | 3 | level | Designer value | gdd.md §2 Mechanic 1 | n/a |
| `LevelMin` | 0 | level | Designer value | gdd.md §2 Mechanic 1 | n/a |
| `LevelMax` | 4 | level | Designer value | gdd.md §2 Mechanic 1 | n/a |
| `BaseDemand` | 2 | cells | Designer value | gdd.md §2 Mechanic 1 | n/a |
| `TutorialDemand` | 1 | cells | Designer value | gdd.md §2 Mechanic 1 (phase 1) | n/a |
| `DemandDeltaNormal` | 1 | cells | Designer value | gdd.md §2 Mechanic 2 | n/a |
| `DemandDeltaSevere` | 2 | cells | Designer value | gdd.md §2 Mechanic 2 | n/a |
| `ShelterSafeLevel` | 2 | level | Designer value | gdd.md §2 Mechanic 4 (below this a radiation card records an exposure) | n/a |
| `HarvestMinLevel` | 3 | level | Designer value | gdd.md §2 Mechanic 5 | n/a |
| `MissionSols` | 31 | sols | Designer value | gdd.md §3 Game Flow | n/a |
| `SeedMin` | 1000 | seed | Designer value | technical_design.md §4 | n/a |
| `SeedMax` | 9999 | seed | Designer value | technical_design.md §4 | n/a |
| `IdleHintSecondsMin` | 6 | seconds | Designer value | gdd.md §2 Mechanic 1 edge cases | n/a |
| `IdleHintSecondsMax` | 8 | seconds | Designer value | gdd.md §2 Mechanic 1 edge cases | n/a |
| `DonkiTimeoutSeconds` | 5 | seconds | Designer value | gdd.md §2 Mechanic 7 | n/a |
| `DonkiRangeDays` | 30 | days | Designer value | gdd.md §2 Mechanic 7 (the DONKI API allows at most 60 days) | n/a |

---

## 2. Numbers on the event-card backs (`text.csv`, `Card_<Id>_Measure`)

Card fronts and reveal text have no numbers; numbers appear only on the back ("What really happened?"). Each was checked against its source on 2026-11-13 (`docs/cards_check.md`).

| Card | Label | Numbers on the card back | Source id |
| :-- | :-- | :-- | :-- |
| Storm2017 | Flown / measured | 11 Sep 2017; more than double any earlier RAD level; more than two days | JPL2017Storm |
| DoseSpike2024 | Flown / measured | 20 May 2024; 8,100 micrograys, like 30 chest X-rays | NASA2024Storm |
| ArtemisIShelter | Flown / measured | 16 Nov 2022; 287 vs 134 µGy per minute, a factor of 2 | George2024EPMC |
| ArtemisIIShelter | NASA plan | Flight Day 8 (8 Apr 2026), demonstration skipped | ArtemisIIAgenda, ArtemisIIFD8 |
| InSight2022 | Flown / measured | 7 Oct 2022; 425 to 275 watt-hours per sol; two weeks | JPLInSight2022 |
| DustStorm2018 | Flown / measured | 10 Jun 2018; tau 10.8 vs above 5.5 in 2007 | JPLOpportunity2018 |
| PowerHookup | NASA plan | about 1 km; Sols 1-3 | SAC21, SAC21Record |
| Apollo13Air | Flown / measured | 14.9 mmHg to 1 mmHg (date 14 Apr 1970) | OCHMOCO2, AFJ13 |
| MOXIE | Flown / measured | 16 runs to 7 Aug 2023; 122 g; up to 12 g per hour; 98 % purity or better | MOXIE2023 |
| Conjunction | Flown / measured | every two years; 28 Aug to 7 Sep 2019 | Conjunction2019 |
| CHAPEA2 | Analog mission on Earth | 378 days; 1,700 square feet; two-week blackout | CHAPEA |
| Lettuce | Flown / measured | 33 days; 10 Aug 2015; half of the leaves | Veggie |
| Pepper | Flown / measured | 137 days; four plants, 26 peppers; 26 Nov 2021 | Pepper2021 |
| LostTomatoes | Flown / measured | two tomatoes, lost 2022, found nearly a year later | Tomatoes2023 |

How much a card raises a system's demand (+1 or +2 cells) is a designer value, not part of the NASA event.

---

## 3. Game rules that are designer decisions

From `gdd.md` §2 and `technical_design.md` §4 (locked by the team on 13.11.2026; tuned by the balance script, child tests still to come).

**Power and levels**
- Power cells per phase: 6 (Commander, ages 10-12) or 4 (Cadet, ages 7-9).
- Level rule: cells given minus demand = +1 or more → level +1; 0 → no change; -1 → level -1; -2 or less → level -2. Levels stay between 0 and 4 (0 failed, 1 Danger, 2 Low, 3 and 4 Good).
- GREENHOUSE changes apply at the start of the next phase ("plants grow slowly").
- AIR at level 0 at the end of a phase ends the mission. Phase 1 cannot be lost.

**Phases and sols (`phases.csv`)**
- Arrival sol 1-3, First walk sol 4-5, Exploration sol 7-10, Resupply and rest sol 11-13, Hard week sol 21-27, Return sol 28-31. Sols outside these ranges are not played. The Hard week range was set by the team because no source range was available.
- Per-phase demands and which phases can draw which card family.

**Designer decisions listed in `technical_design.md` §4**
- Cadet merges Arrival and First walk by skipping First walk; the merged Arrival ends at sol 5. Cadet therefore has no Power-family phase; the Cadet deck guarantees one Radiation and one Comms card.
- In Cadet, one system per phase is locked (column `CadetLockedSystem`: Shelter, Shelter, Greenhouse, Shelter, Greenhouse, Shelter). A locked system needs no cells and keeps its level (no free healing).
- Events are drawn by the phase's event family; if none of that family is left, any allowed card is drawn. The same seed and phase always give the same card.
- Radiation exposure: +1 when the chosen option carries an exposure, or when a Radiation card ends with SHELTER below level 2 (counted once per phase). Exposures are counted, not converted to mSv, because DONKI gives no dose values.
- Mission Report stamps: Crew = lift-off with 0 exposures; Base = lift-off with every system at level 2 or more; Emergency = no system below 2 at the end of any event phase; Bonus = science done or lettuce harvest. All four = "Full mission", otherwise "Minimum mission".
- Mission Control: a reply arrives `AdviceDelayPhases` playable phases later (Commander 1, Cadet 0); nothing is sent or delivered in the blackout phase; held replies arrive in the next non-blackout phase.

**Other designer choices**
- Lettuce harvest only if GREENHOUSE is level 3 or more at the start of the last phase; peppers "won't grow this mission" (real growing times: 33 and 137 days).
- Background radiation in the model: sols elapsed × 0.65 mSv (per-day rate applied per sol).
- Today's Sun panel: last 30 days of DONKI flares, CMEs and geomagnetic storms; 5 s timeout; September 2017 archive when offline. The panel never changes gameplay.
- Balance target (designer value): phase 1 loss 0 %, random-play loss between 20 % and 50 %. Last simulation (1,000 seeds, 14 cards, 13.11.2026): Commander random play 43.9 % loss, Cadet random play 15.5 %, greedy play 0 %, phase 1 loss 0 %. These are computer simulations, not child results.

---

## 4. Time compression

- One mission covers 31 sols (Mars days) and is designed to be played in about 5-8 minutes (design target from `gdd.md`; not yet timed with children).
- Time moves only when the child presses "Next phase". Each phase stands for 2 to 7 sols.
- Mission Control delay is shown, not to scale. A real one-way message between Earth and Mars took 11.014 minutes on 14 Nov 2026 (range 3.2 to 22.25 minutes, 2017-2032; JPL Horizons). In the game a reply arrives one phase later (Commander) or in the same phase (Cadet), and not at all during the blackout phase. The phase delay is a designer choice that makes the waiting visible.

On screen, the time-compression note is the `UI_TimeCompression` line (EN/TR) in `text.csv`, shown at the top of the Source Panel.
