# Touchdown Mars: The Manifest

A Mars mission design game with a first-person mission simulation, made for the **NASA Space Apps Challenge 2026** (challenge: *Space Mission Design Game*) by **Team Touchdown**, İstanbul. For students who are new to space engineering and for anyone curious about how a mission is put together. One player, or a team of up to four on one screen.

A crew of four will live 31 sols at Jezero crater. You design the cargo mission that lands before them: rockets and launches, surface power, radiation shelter, greenhouse, spare parts, science instruments and the link to Earth, under mass, power, budget and data limits. Then you fly it and live it in first person, and a debrief traces every problem back to the design decision that caused it. Then you redesign and fly the same mission again.

**Scope.** The game covers cargo planning and surface operations. Crew transport and return are assumed to be provided. Transfer orbits are not computed.

## Status

14 November 2026. The game loop is in place end to end: design room → launch → surface mission → debrief → redesign. The source defines 29 automation tests (`TD.Design.*` 10, `TD.Model.*` 11, `TD.Ops.*` 8). Play tests with people outside the team are still to come, so no learning claim is made.

## What is in the game

**Act 1: Mission Design Room**
- **Four objectives**, each with its own acceptance criteria, budget and score weights: a first-crew baseline, Science First, Lasting Base and a Sandbox with no budget cap.
- **Seven design slots, 20 options:**
  - Launch: Falcon 9 or Falcon Heavy, with the number of launches.
  - Surface power: fission power units and solar fields.
  - Radiation shelter: three types.
  - Greenhouse: two sizes.
  - Spare parts: two kits.
  - Seven instruments based on flown hardware: Mastcam-Z, SuperCam, PIXL, SHERLOC, MEDA, MOXIE and RAD.
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
- **Transit card:** launches, mass, cruise of about 200 days (NASA; Perseverance took 213) and the cruise and landing hardware. The cargo lands at Jezero before the crew.
- **A connected Jezero base in first person** on Mars gravity (0.38 g, NASA NSSDC). The terrain is a 1 km window of the MRO HiRISE Jezero elevation model. The parts you designed (fission units, solar fields, shelter shielding, relay antenna) appear on the surface.
- **Six phases, sol 1 to sol 31.** In each phase you use the Mission Control console beside the habitat. An objective tracker and a marker show the way.
  - **DECISION:** answer a dated NASA event card on which both choices cost something.
  - **POWER:** place every power cell your design gave you into AIR, GREENHOUSE and SHELTER. In a dust-storm phase solar power falls; fission does not.
  - **CREW PLAN:** give the crew their work hours from 15 tasks, for example suit checkout, off-loading the rover, standing up the weather mast, a science traverse to the delta, baking water out of clay, piling regolith on the shelter, cleaning the solar arrays and a 5-sol rover traverse. Crew day length, the first spacewalk sol and spacewalk durations follow a NASA Mars surface operations study.
  - **GO / NO-GO:** the sols run only when the power is placed, the event is answered and the work is planned.
- **Hands-on work:** take a task yourself and do it by hand outside (hammer stakes, plug in cables, brush dust off the panels, drill a rock core, shovel regolith), or let the time-lapse do it.
- **The base changes with the work:** the rover stays on the lander deck until it is off-loaded, the weather mast stands only after it is deployed, a regolith mound grows on the shelter.
- **Never alone outside:** when you walk away from the hub, a crew member suits up and walks with you.
- **Rest rule:** a spacewalk can be ordered without a rest sol, but that crew member gets less done in the next phase and the debrief lists the broken rule.
- **15 dated NASA event cards**, for example the 2017 solar storm measured by Curiosity's RAD, the 2018 dust storm that ended Opportunity, the 2022 InSight marsquake S1222a, MOXIE, the Apollo 13 air filters and solar conjunction. After each choice the card flips to "What really happened?" with the date, instrument, measurement and source. The design changes what an event costs: shielding lowers the power a radiation event needs, and spares make a repair cheaper.
- **Mission Control delay:** replies arrive one phase late. During solar conjunction no link works, relay or not.
- **Helmet display:** life support, power cells, comms and the Jezero weather of the sol from Perseverance's MEDA station (PDS archive, labelled `ARCHIVE`).
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
- **Languages and controls:** English and Turkish. Keyboard and mouse, or a gamepad.
- **Settings:** Guided or Advanced model, Reduce motion, sound.
- **Privacy:** no accounts, no data collection.

## Known limits

- **Game values.** Module masses, most prices, data volumes, science values, the power cell size, task effects and the budget caps are game values, labelled in the Source Panel. The cruise and landing hardware in the Advanced model is a game assumption per kilogram of landed payload, not a NASA landing efficiency. Curiosity's launch mass breakdown is shown only as a historical example.
- **Reference numbers.** Some sourced numbers (radiation dose rates and the career limit, Earth-Mars light time, Deep Space Network dish sizes) are listed in the Source Panel for context and do not drive a game rule.
- **Time and scale.** 31 sols are played in a few minutes. Mission Control delay is shown as phases, not minutes. No health risk is computed.
- **Team play.** Four roles share one screen; there is no network play.
- **Not in this build.** VR, Moon mode, music and voice.
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
- **Rocks:** the four Megascans rocks are not in this repository (licence). Without them the level opens without rocks. To restore them, add the free Fab listings named below with the Personal licence, import them as glTF (Medium) into `/Game/Fab/Free`, then run `scatter_rocks.py`.

## Data and third-party assets

Full list with URLs, retrieval dates and licences: `Data/Source/sources.csv`.

**NASA data that drives the game**
- NASA CCMC DONKI API: live and archived space weather (Today's Sun panel).
- NASA Mars Trek: MRO HiRISE Jezero DEM (1 m per pixel), a 1 km window near the Perseverance landing site, for the outpost terrain.
- NASA PDS, Perseverance MEDA: Jezero air temperature and pressure for mission sols 1 to 31.
- NASA HEOMD-415, Mars Surface Activities reference mission (NTRS): crew work hours, first spacewalk sol and spacewalk durations.
- NASA BVAD Rev 2: oxygen, water and packaged food per crew member per day, in the cargo mass.
- NASA Science Perseverance instrument pages and the MSL landing press kit: instrument mass and power on the option cards, and Curiosity's launch mass breakdown.
- NASA Kilopower release, NASA MMRTG fact sheet, NASA Photojournal PIA25284: fission output and the loss of solar output to dust.
- NASA Mars Relay Network page: relay data volume per day.
- NASA/JPL Mars 2020 and InSight press kits, NASA Mars mission timeline, NASA Mars EDL white paper (2025), NSSDC Mars fact sheet: transit card, debrief comparison and Mars gravity.
- NASA launch services contract release for Europa Clipper: Falcon Heavy launch price.

**Dated NASA events on the 15 event cards**
- NASA and NASA/JPL mission news pages (2017 and 2024 solar storms at Mars, InSight 2022, Opportunity 2018, MOXIE, solar conjunction, CHAPEA, ISS plant growth), George et al. 2024 on Artemis I radiation, the Artemis II agenda and Flight Day 8 blog, NASA SAC21 (NTRS), NASA OCHMO Carbon Dioxide brief, the Apollo 13 Flight Journal, and the InSight Marsquake Service catalogue V14 with Kawamura et al. 2023.

**Reference values listed in the Source Panel** (context only)
- George et al. 2024 dose rates, NASA OCHMO radiation brief, NASA/JPL Horizons light time, NASA Deep Space Network and telecom pages, NASA Fission Surface Power and KRUSTY, Curiosity RAD Level 2 daily dose, NASA human Mars landing study, NASA ISRU and excavation references.

**Manufacturer data**
- SpaceX Falcon 9 and Falcon Heavy vehicle pages: payload to Mars.

**NASA 3D models**
- NASA 3D Resources: Habitat Demonstration Unit, Z2 Spacesuit, Space Exploration Vehicle, RASSOR and Tall Dish meshes, with every texture removed (no insignia).

**Third-party assets and tools**
- Unreal Engine 5.8 (Epic Games, Unreal Engine EULA).
- Quixel Megascans via Fab (Fab Standard License, Personal tier, free; **not redistributed here**): Icelandic Porous Rock, Icelandic Boulder, Desert Western Cluster Rock Medium 05, Icelandic Sand Rock.
- Sound effects: synthesized by `Scripts/make_sfx.py` (Apache 2.0, not AI-generated).
- Tripo AI (AI-generated 3D models, made in the team's Tripo Studio account; listed in `sources.csv` with `bIsAIGenerated=True`): ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut.
- Design-room pictures: Google Gemini illustrations in one locked style (20 option cards, 4 objective cards, the cold open). Each card shows the tag "AI illustration (Gemini)"; listed in `sources.csv` as `GeminiIllustrations` with `bIsAIGenerated=True`.
- Fonts: Barlow Semi Condensed and IBM Plex Mono (SIL Open Font License 1.1, licence files in `Content/TouchdownMars/UI/Fonts`).
- Editor tooling only, not shipped and not in this repository: Epic's Unreal MCP, VibeUE (MIT), a private team fork of the StraySpark Unreal MCP Server.

No NASA logo, flag, insignia or mission patch is used anywhere in the game.

## AI use

The game concept, the mission architecture, the design decisions and the choice of NASA sources are the team's. We used AI tools as follows:

- **ChatGPT and Claude** were used to draft and revise the team's work plan, to analyse the challenge documents and to set up this repository. Claude was used for competitor analysis and source checking.
- **Anthropic Claude (Claude Code, an AI coding assistant)** wrote the C++ code, the Python editor scripts and the packaging script, built the levels, widgets and DataTables in the Unreal Editor through MCP editor tooling, and drafted the documents, the card research and the English and Turkish game text from the team's research and decisions. Every card fact was checked against its source page.
- **Tripo AI** generated seven 3D models from text prompts written by Claude Code and run in the team's Tripo Studio account: ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut (retopologised and auto-rigged with six preset animations). They are listed in `sources.csv` with `bIsAIGenerated=True`.
- **Google Gemini** generated the 25 design-room illustrations (option cards, objective cards and the cold open of a fictional crew) from prompts written by Claude Code, in one locked style that matches the UI palette. The team checked every image at full size for logos, insignia, flags, mission patches, text and faces and rejected any that had them. Every card carries a visible "AI illustration (Gemini)" tag. Real NASA photos are not used for fictional scenes.
- **Not AI-generated:** the sounds (team script), the NASA data and meshes, and the Megascans rocks. No AI-generated audio or video is used.

## License

Original content is released under the [Apache License 2.0](LICENSE).

This project is not endorsed by NASA.
