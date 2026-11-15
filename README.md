# Touchdown Mars: The Manifest

**Pack the cargo. Live the mission. Trace every problem.**

**[Download for Windows](https://github.com/TouchdownMars/TouchdownMars/releases/download/v1.0-submission/TouchdownMars_Win64_v25.zip)** (64-bit zip, 777 MB, [release v1.0-submission](https://github.com/TouchdownMars/TouchdownMars/releases/tag/v1.0-submission)) · **[Project site](https://touchdownbeyond.earth)** · **[Space Apps project page](https://www.spaceappschallenge.org/2026/find-a-team/touchdown/)** · **Presentation:** [English](https://docs.google.com/presentation/d/1EzAaglJV8fXD3hLVF12cO3WKYD2wkmgN/edit?usp=sharing), [Türkçe](https://docs.google.com/presentation/d/1QmHR3bdQ8BnWaY4VjpNQIqWlAIRcOKwy/edit?usp=sharing) · **[For judges: three minutes](#for-judges-three-minutes)**

[![Touchdown Mars: The Manifest, 30-second trailer (watch on YouTube)](https://img.youtube.com/vi/Z8bS-2ZdFTc/maxresdefault.jpg)](https://www.youtube.com/watch?v=Z8bS-2ZdFTc)

**[▶ Watch the 30-second trailer on YouTube](https://www.youtube.com/watch?v=Z8bS-2ZdFTc)** · [Download the MP4](https://github.com/TouchdownMars/TouchdownMars/releases/download/v1.0-submission/TouchdownMars_Trailer_30s.mp4) · *Contains AI-generated 3D models (Tripo AI), video (Google Gemini), voice, sound effects and music (ElevenLabs).*

A Mars mission design game with a first-person mission simulation, made for the **NASA Space Apps Challenge 2026** (challenge: *Space Mission Design Game*) by **Team Touchdown**, İstanbul. For students who are new to space engineering and for anyone curious about how a mission is put together. One player, or a team of up to four on one screen.

A crew of four will live 31 sols at Jezero crater. You design the cargo mission that lands before them: rockets and launches, surface power, radiation shelter, greenhouse, spare parts, science instruments and the link to Earth, under mass, power, budget and data limits. Then you launch it and live the mission in first person, and a debrief traces every problem back to the design decision that caused it. Then you redesign and fly the same mission again.

**Scope.** The game covers cargo planning and surface operations. Crew transport and return are assumed to be provided. Transfer orbits are not computed. Landing the cargo before the crew follows NASA's SAC21 human Mars architecture (Rucker et al. 2022, section "Pre-Deployed Cargo", p. 4) and the HEOMD-415 surface reference mission, in which surface power is running before the crew lands (p. 13).

## Mission definition

**Spacecraft.** The mission is a cargo flight, not one probe. What the player designs is a set of launch packages, each on its own rocket:

| Part | In the game | Numbers |
| :-- | :-- | :-- |
| Launch vehicle | Falcon 9 or Falcon Heavy, with the number of launches | Payload to Mars 4,020 kg and 16,800 kg (SpaceX vehicle pages) |
| Cruise and landing hardware | Added to every landed package in the Advanced model | 2.67 kg per kg of landed payload, a labelled game assumption scaled from Curiosity's entry, descent and landing system (MSL landing press kit; the cruise stage is not counted) |
| Landed payload: power | Fission power units and solar fields | Output from the NASA Kilopower release; solar loss in dust after InSight |
| Landed payload: habitat systems | Radiation shelter, greenhouse, spare parts | Game masses and prices, labelled |
| Landed payload: science | Up to eight instruments based on flown hardware | Mass and power from NASA instrument pages; data and science value are game values |
| Landed payload: crew supplies | Oxygen, water and packaged food for four crew and 31 sols | NASA BVAD Rev 2 |
| Communications | A direct antenna on the surface, or a relay orbiter that stays in Mars orbit and needs no landing hardware | Relay data volume from the NASA Mars Relay Network page |

A module cannot be split between launches, so the design review checks every package against its rocket. The game does not model batteries, the on-board computer, attitude control or the structure as separate parts.

**Orbit and trajectory.** The trajectory is fixed and is not a player choice. Every launch flies the same Earth-Mars transfer: a cruise of about 200 days in a launch window that opens about once every 26 months (NASA Mars mission timeline), with a landing at Jezero crater. The team fixed it because the game does not compute transfer orbits, and a choice of transfer without that computation would be a made-up number. The player's orbit decisions are these two:

- **Rocket and launch count.** Each rocket's payload to Mars is the limit for its package. Cargo that does not fit blocks the launch.
- **Relay orbiter or direct link.** The relay orbiter is the one spacecraft the player places in Mars orbit. It returns more data per sol than the direct antenna, costs mass and money, and cannot help during solar conjunction.

**Mission lifetime.** The lifetime is fixed and is the same for every objective.

| Stage | Length | Source |
| :-- | :-- | :-- |
| Cargo cruise | About 200 days (Perseverance took 203: launch 30 July 2020, landing 18 February 2021) | NASA Mars mission timeline; Mars 2020 landing press kit, Quick Facts |
| Crew surface mission | 31 sols | Game value |
| Phases played | Arrival (sols 1-3), First walk (4-5), Exploration (7-10), Resupply and rest (11-13), Hard week (21-27), Return (28-31) | Game scenario (`Data/Source/phases.csv`) |
| Solar conjunction | Sols 21-27, no link to Earth | Game scenario; the event is from NASA's solar conjunction page |

The lifetime sets the crew supplies in the cargo mass, the heater load from 31 sols of MEDA air temperature and the crew dose summed from 31 real RAD days.

## Challenge elements and where they are in the game

The challenge names twelve mission elements. Each one is listed here with its place in the game.

| Challenge element | In the game | Where |
| :-- | :-- | :-- |
| Objective | Four objectives with their own score weights | Act 1, objective cards; `Data/Source/objectives.csv` |
| Requirements | Acceptance criteria for each objective, checked in the debrief | Act 1 and Act 3 |
| Spacecraft design | Launch packages, landed systems and the relay orbiter (see Mission definition) | Act 1, seven design slots |
| Scientific instruments | Eight instruments with mass, power, cost, data and science value | Act 1, Science Lead |
| Launch vehicle | Falcon 9 or Falcon Heavy, number of launches, fit check for each package | Act 1, Mission Planner; design review |
| Orbit | Fixed transfer; relay orbiter or direct link | Mission definition; Act 1, Comms & Budget |
| Budget | A cap for each objective; none in Sandbox | Dashboard; design review |
| Mass | Mass to Mars against the launch capacity | Dashboard; design review |
| Power | Power cells in clear sky and in a dust storm, placed by hand in every phase | Dashboard; Act 2, POWER |
| Communications | Direct antenna or relay orbiter; Mission Control replies one phase late; conjunction blackout | Act 1; Act 2 |
| Data | Data per sol, stored until a link is open; only data that reaches Earth scores | Dashboard; Act 3, science value |
| Mission lifetime | About 200 days of cruise and 31 sols in six phases | Transit card; Act 2 |

Also from the challenge text:

| Challenge ask | In the game |
| :-- | :-- |
| Up to four participants with roles | Four roles on one screen, each with its own systems and goal |
| Simulation and evaluation | A seeded surface mission, then a debrief with four score axes |
| Feedback on performance | "What broke" names the design decision behind each problem |
| Redesign and fly again | Redesign on the same seed |
| Different knowledge levels | Guided and Advanced model; the first flight is a tutorial |
| Not built | Network play, competing teams and a leaderboard |

## For judges: three minutes

1. **Download:** the Windows build, `TouchdownMars_Win64_v25.zip` (777 MB), from the release [v1.0-submission](https://github.com/TouchdownMars/TouchdownMars/releases/tag/v1.0-submission), built from commit `7469f77` (later commits add development flags for trailer footage, which are not in the shipping build, and a Mac compile fix with the same behaviour). Unzip it and run `TouchdownMars\TouchdownMars.exe`. A Mac (Apple Silicon) zip is in the same release.
2. **First minute:** take the console, pick an instrument, poll the room and press LAUNCH; the pre-launch checklist says what is still missing.
3. **On Mars:** follow the red marker to the Mission Control console beside the habitat (E or X), place the power cells, answer the event, run the sols. Hold E or X at a lit work spot to do a task by hand.

## Status

15 November 2026. The game loop is in place end to end: design room → launch → surface mission → debrief → redesign. The source defines 30 automation tests (`TD.Design.*` 10, `TD.Model.*` 12, `TD.Ops.*` 8); 30 of 30 passed on 15 November 2026 at commit `5febe2b` (result lines in `Evidence/tests_2026-11-15.txt`). On 15 November 2026 eight adult volunteers from outside the team played the game and answered a short anonymous form; the results, the method and the limits are in `Evidence/playtest/`. No learning claim is made.

## Verified and planned

| Claim | Status | Evidence |
| :-- | :-- | :-- |
| Same seed and one design change give the same event list | Verified | `Evidence/runs/` (two run records, equal `event_list_hash`) |
| Mission Control replies one phase late and names the system that was weakest when you called | Verified | test `TD.Model.AdviceRemembersTheStateAtSend`, `TD.Model.MissionControlDelay`; `advice_stale` in the run records |
| Guided and Advanced fly the same six phases and the same delay | Verified | automatic Guided flight, 14 Nov 2026 (six phases, mission completed) |
| Every event card fact was checked against its source page | Verified | `Evidence/cards_check.md` |
| Crew dose is summed from real Curiosity RAD days | Verified | `Data/Source/rad_dose.csv`; `crew_dose_ugy_total_after` in the run records |
| Play tests with people outside the team | Done, 8 players (15 Nov 2026) | `Evidence/playtest/` (anonymous answers, coding rules, observer notes, limits) |

## What is in the game

**Act 1: Mission Design Room**
- **Four objectives**, each with its own acceptance criteria, budget and score weights: a first-crew baseline, Science First, Lasting Base and a Sandbox with no budget cap.
- **Seven design slots, 21 options:**
  - Launch: Falcon 9 or Falcon Heavy, with the number of launches.
  - Surface power: fission power units and solar fields.
  - Radiation shelter: three types.
  - Greenhouse: two sizes.
  - Spare parts: two kits.
  - Eight instruments based on flown hardware: Mastcam-Z, SuperCam, PIXL, SHERLOC, MEDA, MOXIE, RAD and the InSight SEIS seismometer (its mass is not counted: no NASA mass figure was found).
  - Link to Earth: direct antenna or relay orbiter.
  - Every option card says what it is, why it matters, the hardware it is based on, and whether its numbers are NASA or manufacturer values or game values.
- **Live mission dashboard:**
  - **Mass to Mars:** hardware, crew food, water and oxygen from NASA BVAD for a crew of four, and, in the Advanced model, cruise and landing hardware as a labelled game assumption. Each launch is a separate package, and a module cannot be split between launches.
  - **Power cells:** in clear sky and in a dust storm.
  - **Budget, data per sol and science value.** Science data waits in storage for an open link.
- **Design review:** red lines block the launch, for example cargo that does not fit the launch packages. Amber lines are risks you may accept.
- **Known risks before launch:** solar conjunction, dust storms, solar storms and failures.
- **Concept maturity meter**, after JPL's Concept Maturity Levels.
- **Four roles on one screen:** Mission Planner, Systems Engineer, Science Lead, Comms & Budget.
  - Each role changes only its own systems and has its own goal.
  - A pre-launch checklist shows empty slots and open red lines. Each role calls GO, and any change clears the poll.
  - One player can play all four roles.
- **First flight:** the first flight of a session is the tutorial. A short cold open shows a fictional solar-only crew losing power in a dust storm, then guided steps teach the cause-and-effect logic of the design room.

**Act 2: Fly and live**
- **Transit card:** launches, mass, cruise of about 200 days (NASA; Perseverance took 203) and the cruise and landing hardware. The cargo lands at Jezero before the crew.
- **A connected Jezero base in first person** on Mars gravity (0.38 g, NASA NSSDC). The terrain is a 1 km window of the MRO HiRISE Jezero elevation model. The parts you designed (fission units, solar fields, shelter shielding, relay antenna) appear on the surface.
- **Six phases, sol 1 to sol 31.** In each phase you use the Mission Control console beside the habitat. An objective tracker and a marker show the way.
  - **DECISION:** answer a dated NASA event card on which both choices cost something.
  - **POWER:** place every power cell your design gave you into AIR, GREENHOUSE and SHELTER. In a dust-storm phase solar power falls; fission does not.
  - **CREW PLAN:** give the crew their work hours from 17 tasks, for example suit checkout, off-loading the rover, standing up the weather mast, a science traverse to the delta, baking water out of clay, piling regolith on the shelter, cleaning the solar arrays and a 5-sol rover traverse. Crew day length, the first spacewalk sol and spacewalk durations follow a NASA Mars surface operations study.
  - **GO / NO-GO:** the sols run only when the power is placed, the event is answered and the work is planned.
- **Hands-on work:** take a task yourself and do it by hand outside (hammer stakes, plug in cables, brush dust off the panels, drill a rock core, shovel regolith), or let the time-lapse do it.
- **The base changes with the work:** the rover stays on the lander deck until it is off-loaded, the weather mast stands only after it is deployed, a regolith mound grows on the shelter.
- **Never alone outside:** when you walk away from the hub, a crew member suits up and walks with you.
- **Rest rule:** a spacewalk can be ordered without a rest sol, but that crew member gets less done in the next phase and the debrief lists the broken rule.
- **15 dated NASA event cards**, for example the 2017 solar storm measured by Curiosity's RAD, the 2018 dust storm that ended Opportunity, the 2022 InSight marsquake S1222a, MOXIE, the Apollo 13 air filters and solar conjunction. After each choice the card flips to "What really happened?" with the date, instrument, measurement and source. The design changes what an event costs: shielding lowers the power a radiation event needs, and spares make a repair cheaper.
- **Mission Control delay:** replies arrive one phase late. The reply names the system that was weakest when you called; by the time it arrives it may be out of date, and the debrief counts such replies. During solar conjunction no link works, relay or not.
- **Helmet display:** life support, power cells, the crew's radiation dose, comms and the Jezero weather of the sol from Perseverance's MEDA station (PDS archive, labelled `ARCHIVE`).
- **Heating from MEDA:** the daily air temperatures set the habitat heater load (game heat-loss model), which is part of the power cells a design must carry.
- **Crew radiation dose from Curiosity RAD:** each sol uses one real day of RAD dosimetry (CDAWeb, Aug-Oct 2017). Outside time gets the full rate; the habitat and the storm shelter keep part of it out (labelled game assumptions). A radiation card replays the 11-17 Sep 2017 solar event, and the decision card shows the event-day dose for a spacewalk versus the shelter. The mSv value is an estimate scaled to the average Mars-surface dose equivalent and compared with the NASA career limit.
- **Seismometer (InSight SEIS, up to 8.5 W):** level it outside, then study real records from the InSight Marsquake Service catalogue V14 in the lab, one science walk each. The records are what InSight measured in Elysium Planitia, not shaking at the base.
- **Today's Sun:** live NASA DONKI space weather labelled `LIVE` with the fetch time, or the September 2017 archive labelled `ARCHIVE`. It never changes gameplay.
- **Outcome shots:** short, skippable cinematics that show each result in the scene.

**Act 3: Mission Debrief**
- **Acceptance criteria first:** each requirement of the objective, met or missed.
- **Four score axes, then a total weighted by the objective:** mission success, science value (only data that reaches Earth counts), efficiency (counts only after a successful lift-off) and decisions.
- **"What broke"** names the design decision behind each problem.
- **Comparisons:** this flight against the one before, with what you changed; your flights this session; and real NASA missions (Perseverance, Curiosity, InSight, landed mass of flown and human-class landers).
- **Team debrief:** which role goals held.
- **Redesign** on the same seed, or choose a new objective.

**Everywhere**
- **Source Panel:** every number is listed with its source and where in the source it is, or labelled as a game value.
- **Languages and controls:** English (the Turkish text is kept in `text.csv` but switched off in this build). Keyboard and mouse, or a gamepad.
- **Settings:** Guided or Advanced model, Reduce motion, sound.
- **Privacy:** no accounts, no data collection. The game writes a local run record file for each flight (`Saved/Runs/`); nothing leaves the machine.

## Known limits

- **Game values.** Module masses, most prices, data volumes, science values, the power cell size, task effects and the budget caps are game values, labelled in the Source Panel. The cruise and landing hardware in the Advanced model is a game assumption per kilogram of landed payload, not a NASA landing efficiency. Curiosity's launch mass breakdown is shown only as a historical example.
- **Reference numbers.** Some sourced numbers only feed a display (the mSv estimate and the share of the career limit, today's Earth-Mars light time) or are listed in the Source Panel for context (Deep Space Network dish sizes and the other values in group 4 of Data and third-party assets); they do not change the outcome.
- **Time and scale.** 31 sols are played in a few minutes. Mission Control delay is shown as phases, not minutes. Crew dose is summed from real RAD days, but the habitat and shelter factors are game assumptions and no health effect is computed.
- **Team play.** Four roles share one screen; there is no network play.
- **Not in this build.** VR and voice.
- **SAC21 is an analysis option**, labelled "NASA plan" in the game; it is not a committed NASA mission.

## Tech

- Unreal Engine 5.8, C++ game module `TouchdownMars`, Slate and UMG, Enhanced Input.
- Git LFS for Unreal assets. Run `git lfs install` once before cloning.
- The `.uproject` file lives at the repository root.

### Repository layout

| Path | What it holds |
| :- | :- |
| `Source/TouchdownMars/` | C++: design rules, mission rules and base operations (`Model/`), tests (`Tests/`), subsystems and DONKI client (`Systems/`), scene actors, astronaut, work spots and camera (`Actors/`), design room, Mission Control and widgets (`UI/`) |
| `Data/Source/` | CSV source of truth for every DataTable (mission options, objectives, phases, events, tasks, sites, constants, sources, text EN/TR, Jezero weather, DONKI archive, outcome shots) |
| `Content/TouchdownMars/` | Unreal assets: DataTables, widgets, maps (`L_TD_Outpost`, `L_TD_Test`), materials, sounds, NASA 3D meshes |
| `Scripts/` | Python editor scripts (data import, UI and level setup, terrain, art, cameras, base operations scene, balance check, sound synthesis) and `package.ps1` |
| `Media/` | The 30 s submission trailer, `TouchdownMars_Trailer_30s.mp4` (narration and music: ElevenLabs, see AI use), and the team presentation in English and Turkish (`TouchdownMars_Presentation_EN.pptx`, `TouchdownMars_Presentation_TR.pptx`) |

### Build and run

1. Install Unreal Engine 5.8 and Visual Studio 2022 with the "Game development with C++" workload.
2. `git lfs install`, then clone this repository.
3. Open `TouchdownMars.uproject` and let Unreal build the module (or run `Build.bat TouchdownMarsEditor Win64 Development -Project=<path>/TouchdownMars.uproject`).
4. Play `L_TD_Outpost` (the default game map).
5. Package a Windows build: `powershell -ExecutionPolicy Bypass -File Scripts/package.ps1 -Name MyBuild` (close the editor first).

Tests: `UnrealEditor-Cmd.exe TouchdownMars.uproject -ExecCmds="Automation RunTests TD.;Quit" -unattended -nullrhi -testexit="Automation Test Queue Empty"` (design rules `TD.Design.*`, mission rules `TD.Model.*`, base operations `TD.Ops.*`). Development builds also accept `-TDAutoPlay` (plays the surface mission to the debrief) and the console command `td.AutoPlay`.

Data changes: edit the CSV in `Data/Source/`, then run `Scripts/import_data.py` in the editor (Python console or `exec(open(...).read())`).

### Rebuilding the outpost level

Scripts in this order (inside the editor, Play-in-Editor stopped): `setup_outpost.py` → `setup_terrain.py` → `make_ground_material.py` → `import_nasa_models.py` → `setup_art.py` → `scatter_rocks.py` → `setup_cameras.py` → `setup_props.py` → `setup_tripo.py` → `setup_base.py` (connected base) → `setup_ops.py` (parts that appear with the crew's work) → `setup_fps.py`.

- The terrain, rock and camera scripts call the free [VibeUE](https://github.com/kevinpbuckley/VibeUE) editor plugin (MIT). Install it into `Plugins/VibeUE/`; it is not part of this repository.
- `setup_terrain.py` needs the NASA Mars Trek elevation file (see `Scripts/dem_to_heightmap.py`); the generated heightmap is in `Data/Terrain/`.
- **Rocks and ground:** the Megascans rocks and ground surfaces are not in this repository (licence). Without them the level opens without rocks and with the plain regolith material. To restore them, add the free Fab listings named below with the Personal licence, import them as glTF (Medium) into `/Game/Fab/Free`, then run `scatter_rocks.py`.

## Data and third-party assets

`Data/Source/sources.csv` is the full ledger: 131 rows and 95 distinct URLs (the 36 Tripo rows share one URL, and the Gemini illustration and video rows share another), each with its retrieval date, licence and use. Its `Kind` column says what a source's numbers are: Measured (42 rows), Plan (12: NASA studies and targets, not flown results), Manufacturer (2: SpaceX) or Reference (75: assets, tools and context pages). Of the 211 constants in `constants.csv`, 104 carry a source and the place in that source; 107 are game values and are labelled as game values in the Source Panel.

The groups below follow what each source does in the game. A source counts as used only when its data is in a file that the code reads, or when it is an asset in the level.

**1. Data that sets a design limit or a surface rule**
- NASA BVAD Rev 2, Table 3-31: oxygen, potable water and packaged food per crew member per day (`constants.csv`). `TDMissionDesign.cpp` adds them to the cargo mass for a crew of four; the design review blocks the launch when the cargo is heavier than the rockets can send.
- SpaceX Falcon 9 and Falcon Heavy vehicle pages (manufacturer values): payload to Mars, the launch capacity in `TDMissionDesign.cpp`.
- NASA release 21-098 (Europa Clipper launch services contract): the Falcon Heavy launch price in the budget. The Falcon 9 price is a game value.
- NASA Science, Perseverance science instruments: mass and power of Mastcam-Z, SuperCam, PIXL, SHERLOC, MEDA and MOXIE. MSL landing press kit: RAD mass. NASA InSight instrument page: SEIS power (its mass is not counted). All through `mission_options.csv` into the mass and power budget.
- NASA release 18-031 (Kilopower): 10 kWe per fission unit, turned into power cells. Unit mass and price, and the solar field output, mass and price, are game values.
- NASA Photojournal PIA25284: InSight fell from about 5,000 to about 500 Wh per sol. The game value `DustStormSolarShare` (0.1, the share of solar output left in a dust-storm phase) is scaled after it.
- NASA Mars Relay Network page: MRO relay volume per day, the data per sol of the relay option. The direct-link volume is a game value.
- NASA HEOMD-415 (NTRS 20220000589): crew work hours per sol, the first spacewalk sol, spacewalk hours and task durations (`constants.csv`, `tasks.csv`); `TDOperations.cpp` and `TDMissionSubsystem.cpp` limit the crew plan with them.
- Russell and Klaus 2007: 3.0 crew-hours per day of life-support maintenance; the maintenance task length (a game value) is set from it.
- Perseverance MEDA (NASA PDS): daily air temperature for mission sols 1 to 31 (`mars_weather.csv`). `TDMissionSubsystem.cpp` turns it into the habitat heating load with a game heat-loss model, which raises the power cells a design must carry. The weather is also shown on the helmet display.
- NSSDC Mars Fact Sheet: 0.38 g for first-person walking (`TDAstronautCharacter.cpp`, `TDWorkSpot.cpp`).
- InSight Marsquake Service catalogue V14: the ten largest events (`marsquakes.csv`), studied one by one in the seismometer task; each counts as a science walk.

**2. Data that builds the world or is shown in the game, without changing the outcome**
- NASA Mars Trek, MRO HiRISE Jezero DEM (1 m per pixel): `Scripts/dem_to_heightmap.py` makes the heightmap in `Data/Terrain/`, the outpost terrain (a 1 km window centred at 18.4447 N, 77.4508 E). The base pad is levelled (designer change).
- NASA GISS Mars24 algorithm: `TDSkySubsystem.cpp` computes the sun position over the base for the phase date. The day length on screen is a designer value.
- Curiosity RAD Level 2 daily dose (SPDF CDAWeb, 20 Aug to 9 Oct 2017; `rad_dose.csv`): each sol adds one real day to every crew member's dose (`TDMissionSubsystem.cpp`). The habitat, shelter and berm factors are game assumptions. The dose is shown on the helmet, in the Mission Control console (including the event-day dose for a spacewalk versus the shelter) and in the run record; it does not change the score and no health effect is computed. George et al. 2024 (average Mars-surface dose equivalent) scales the mSv estimate; the NASA OCHMO career limit is shown for comparison only.
- NASA CCMC DONKI: the Today's Sun panel, live (`TDDonkiClient.cpp`) or the stored September 2017 archive (`donki_archive.csv`). It never changes gameplay.
- NASA/JPL Horizons: today's Earth-Mars light time on the Mission Control console. The Mission Control delay in the game is counted in phases.
- Transit card and debrief comparison (`TDDesignRoom.cpp`): NASA Mars mission timeline (cruise of about 200 days), Mars 2020 landing quick facts (launch and landing dates, 203 days; rover mass and cost), MMRTG fact sheet (110 W), MSL landing press kit (Curiosity masses), InSight landing press kit (cost), PIA25284 (InSight energy per sol) and the NASA Mars EDL white paper 2025 (landed mass of flown and human-class landers).
- Kleinhenz and Paz (NTRS 20170005179) and Mueller et al., RASSOR 2.0 (NTRS 20210011366): clay water content and digging rate. They give the kilograms of water and regolith shown on the console after the water and berm tasks (game values derived from them); the task effects are game values.

**3. Event cards (15).** Each card shows the date, instrument, measurement and source on its "What really happened?" side; the cost of each choice is a game value. Facts checked in `Evidence/cards_check.md`.
- Radiation: NASA/JPL 2017 solar storm at Mars (Curiosity RAD); NASA 2024 solar storm at Mars; George et al. 2024 (Artemis I storm shelter); NASA Artemis II daily agenda and Flight Day 8 blog.
- Power: NASA/JPL InSight waits out dust storm (2022); NASA/JPL Opportunity hunkers down (2018); NASA SAC21 human Mars architecture (Rucker et al. 2022) and its NTRS record, labelled "NASA plan".
- Equipment: NASA OCHMO carbon dioxide brief and the Apollo 13 Flight Journal; NASA MOXIE completes Mars mission (2023); InSight Marsquake Service catalogue V14 and Kawamura et al. 2023 (S1222a).
- Comms: NASA, What's Mars Solar Conjunction (2019 moratorium); NASA CHAPEA, labelled "Analog mission on Earth".
- Food: NASA Veggie fact sheet; NASA second pepper harvest (2021); NASA lost ISS tomatoes (2023).

**4. Reference values in the Source Panel only** (no rule and no other screen reads them): Horizons shortest and longest light time; NASA Goddard Space Communications, 7 Things; NASA Deep Space Network; NASA/JPL MRO data milestone (2010); Mars 2020 telecommunications press kit; KRUSTY test results (NTRS 20180005435); NASA Fission Surface Power; NASA/JPL Opportunity recovery (dust optical depth); Cianciolo and Polsgrove, human Mars EDL study (NTRS 20160011571).

**5. Look and sound references.** No number from these is shown in the game; sizes, counts and levels are look values. They are not rows in `sources.csv`.
- Bell et al. 2022, Science Advances 8, eabo4856 (doi:10.1126/sciadv.abo4856): dust optical depth about 0.45 at Jezero for the sky (`setup_environment.py`); most raised rocks under 30 cm (`scatter_pebbles.py`).
- Lemmon et al. 2004, Science 306 (doi:10.1126/science.1104474): dust scale height of about 11 km for the sky (`setup_environment.py`).
- Lemmon et al. 2022, GRL 49(17) (doi:10.1029/2022GL100126): the dust-storm sky multiplier (`TDSkySubsystem.cpp`).
- Tomasko et al. 1999, JGR Planets 104(E4) (doi:10.1029/1998JE900016): forward-scattering dust in the air (`setup_graphics.py`, `TDDustFXSubsystem.cpp`).
- Newman et al. 2022, Science Advances 8, eabn3783 (doi:10.1126/sciadv.abn3783): daytime wind direction and dust devil hours (`TDDustFXSubsystem.cpp`).
- NASA Photojournal PIA26074, Martian Whirlwind Takes the Thorofare: dust devil drift speed (`TDDustFXSubsystem.cpp`).
- NASA Photojournal PIA19400, Sunset in Mars' Gale Crater: colour of the blue glow around a low sun (`make_sky_glow.py`).
- Maurice et al. 2022, Nature 605 (doi:10.1038/s41586-022-04679-0): Mars sound is quiet and loses its high frequencies within metres, the basis of the four-layer sound mix (`TDSoundscapeSubsystem.h`).
- Polsgrove et al. 2017 (NTRS 20170003391) and 2019 (NTRS 20190001229), NASA human Mars Ascent Vehicle studies: the shape of the crew ascent vehicle model at LZ-1 (`setup_models4.py`).
- Golombek et al. 2021: the shape of the rock size-frequency model behind the pebble density (`scatter_pebbles.py`).

**6. Read but not used by the game** (marked "Reference only" in `sources.csv`): MSL REMS PDS bundle; Curiosity REMS daily weather feed and InSight weather feed (their tables are in `Data/Reference/`, which no code reads); InSight TWINS and PS PDS bundles; the Mars 2020 launch press kit, Mission page (its 213-day trip is for a planned 20 July 2020 launch; the game uses the 203-day flight); Zeitlin et al. 2018; Hassler et al. 2014.

**7. Design references (challenge resources).** JPL Concept Maturity Levels (Wessen et al., AIAA 2013): the four levels of the maturity meter (`TDMissionDesign.cpp`). NASA Design and Develop Science Missions (2025) and the NASA GSFC Mission Design Lab: the design, simulate, evaluate, redesign loop and the disciplines behind the four roles. No number in the game comes from them. GMAT is not used: transfer orbits are not computed (see Known limits).

**8. Assets in the game**
- NASA 3D Resources, nine meshes with every texture removed (no insignia): Habitat Demonstration Unit, Z2 Spacesuit, Space Exploration Vehicle, RASSOR, Tall Dish, Radome, Hammer, Wrench and Pistol Grip Tool.
- Quixel Megascans via Fab (Fab Standard License, Personal tier, free). The source assets are **not redistributed in this repository**; ten are cooked into the packaged Windows build as part of the game: Icelandic Porous Rock, Icelandic Boulder, Desert Western Cluster Rock Medium 05, Icelandic Sand Rock, Desert Western Pile Rock Small 01, Stones and Pebbles Pack, Gravel on Soil, Dry Soil Ground, Dry Rippled Sand and Painted Metal. An eleventh listing in `sources.csv`, Red Desert Sand, is not used.
- Tripo AI: 36 AI-generated 3D models in `Content/TouchdownMars/Tripo/`, each placed by a setup script or loaded by the game code (see AI use). The first ascent vehicle model is kept in the project, but since 15 November its actor shows the newer crew ascent vehicle mesh.
- Google Gemini: 25 design-room illustrations (20 option cards, 4 objective cards, the cold open), each tagged "AI illustration (Gemini)"; and four AI-generated cutscene videos made with Gemini (Veo) in `Content/Movies` (launch from Earth, landing at Jezero, lift-off from Mars, return to Earth), each skippable and tagged "AI video (Google Veo)" on screen.
- Sound: 50 sound effects and 6 music cues in `Data/Audio`. Fifteen sounds (the UI sounds, alarm, card flip, stamp, radio call, system up and down, and the latch, plug, scan, switch and done work sounds) are synthesized by `Scripts/make_sfx.py` (Apache 2.0, not AI-generated). Thirty-five sounds are AI-generated with ElevenLabs Sound Effects in the team's ElevenLabs account (`ElevenLabsSFX`, `bIsAIGenerated=True`): Mars wind, dust storm, habitat hum, suit breathing, regolith footsteps, lift-off and the drill, hammer, ratchet, scoop and brush work sounds, plus the in-helmet set (suit fan, heavy breathing, suit fabric, boot contact thumps, storm buffeting, sand on the visor, caution tone, radio squelch) and the rover, excavator and power-unit machine sounds. The six instrumental music cues (exploring, dust storm, design screen, lift-off) are AI-generated with ElevenLabs Music (`ElevenLabsMusic`, `bIsAIGenerated=True`).
- Fonts: Barlow Semi Condensed and IBM Plex Mono (SIL Open Font License 1.1, licence files in `Content/TouchdownMars/UI/Fonts`); Roboto, Unreal's default UI font (Apache 2.0).
- Unreal Engine 5.8 (Epic Games, Unreal Engine EULA).

**9. Tools, not shipped and not in this repository:** Claude Code (Anthropic), Epic's Unreal MCP, VibeUE (MIT) and a private team fork of the StraySpark Unreal MCP Server. Tripo Studio, ElevenLabs and Google Gemini made the assets above (see AI use).

No NASA logo, flag, insignia or mission patch is used anywhere in the game.

## AI use

The game concept, the mission architecture, the design decisions and the choice of NASA sources are the team's. We used AI tools as follows:

- **ChatGPT and Claude** were used to draft and revise the team's work plan, to analyse the challenge documents and to set up this repository. Claude was used for competitor analysis and source checking.
- **Anthropic Claude (Claude Code, an AI coding assistant)** wrote the C++ code, the Python editor scripts and the packaging script, built the levels, widgets and DataTables in the Unreal Editor through MCP editor tooling, and drafted the documents, the card research and the English and Turkish game text from the team's research and decisions. Every card fact was checked against its source page.
- **Tripo AI** generated 36 3D models from text prompts run in the team's Tripo Studio account. The first seven, from prompts written by Claude Code: ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut (retopologised and auto-rigged with six preset animations; its textures also dress the crew suits). Twenty more across the outpost: corridor tubes, habitat modules, airlock, light masts, regolith mounds, weather mast, water wall frames, ISRU plant, storage tanks, the Mission Control console body, payload cases, junction boxes, a field tablet, a sample scoop, solar array tables, an insulated pipe run, an oxygen production unit, a corridor end bulkhead, the crew ascent vehicle on its lander and the cargo lander. Six small kit models mark the hands-on work spots: a service panel, a core drill rig, a survey stake, an anchor plate, a regolith hopper and a test solar panel. Two hard-shell habitat modules replace the Habitat Demonstration Unit copy at the life-support (B2) and lab (B3) modules, and a rigid corridor segment, repeated in rows, makes the pressurised corridors. All are listed in `sources.csv` with `bIsAIGenerated=True`.
- **Google Gemini** generated the 25 design-room illustrations (option cards, objective cards and the cold open of a fictional crew) from prompts written by Claude Code, in one locked style that matches the UI palette. The team checked every image at full size for logos, insignia, flags, mission patches, text and faces and rejected any that had them. Every card carries a visible "AI illustration (Gemini)" tag. Real NASA photos are not used for fictional scenes.
- **ElevenLabs Sound Effects** generated 35 sound effects from text prompts written by Claude Code in the team's ElevenLabs account: Mars wind, dust storm, habitat hum, suit breathing, regolith footsteps, lift-off, the drill, hammer, ratchet, scoop and brush work sounds, the in-helmet set (suit fan, heavy breathing, suit fabric, boot contact thumps, storm buffeting, sand on the visor, caution tone, radio squelch) and the rover, excavator and power-unit machine sounds. They are listed in `sources.csv` as `ElevenLabsSFX` with `bIsAIGenerated=True`.
- **ElevenLabs Music** generated the six instrumental music cues (exploring, dust storm, design screen, lift-off) from text prompts written by Claude Code in the team's ElevenLabs account. They are listed in `sources.csv` as `ElevenLabsMusic` with `bIsAIGenerated=True`; the music under the 30 s submission trailer is made the same way.
- **ElevenLabs text to speech** voiced the narration of the 30 s submission trailer with a stock synthetic narrator voice (not a clone of any person); the game itself has no voice. Listed in `sources.csv` as `ElevenLabsVoice` with `bIsAIGenerated=True`.
- **Google Gemini (Veo video)** generated the four cutscene videos (launch from Earth, landing at Jezero, lift-off from Mars, return to Earth) from prompts written by Claude Code, using the game's own frames, models and design-room art as references so they match the game. They are listed in `sources.csv` as `GeminiVeoCutscenes` with `bIsAIGenerated=True`; every cutscene carries an on-screen "AI video (Google Veo)" tag and can be skipped.
- **Not AI-generated:** the UI sounds and the remaining work sounds (team script), the NASA data and meshes, and the Megascans rocks. The game has no voice; the only AI voice is the trailer narration above.

## License

Original content is released under the [Apache License 2.0](LICENSE).

This project is not endorsed by NASA.
