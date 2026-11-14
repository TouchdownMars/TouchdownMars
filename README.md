# Touchdown Mars

A Mars mission design game with a first-person mission simulation, in development for the **NASA Space Apps Challenge 2026** (challenge: *Space Mission Design Game*) by **Team Touchdown**, İstanbul. For beginner to intermediate players, classrooms and families; one player or a team of up to four on one screen.

**Design it. Fly it. Live it.** A crew of four will live 31 sols at Jezero crater. Your team designs the cargo mission that lands before them: rockets and launches, surface power, radiation shelter, greenhouse, spare parts, science instruments and the link to Earth, under real mass, power, budget and data limits. Then you launch it and live it in first person, and a debrief traces every problem back to the decision that caused it.

## Status

Work in progress during the hackathon (14–15 November 2026). Windows and macOS (Apple silicon) builds run end to end: design room → launch → surface mission → debrief → redesign. Automated tests: 17 pass (`TD.Model.*`, `TD.Design.*`). Play tests with people outside the team are still to come, so no learning claim is made yet.

## What is in the game now

**Act 1: Mission Design Room**
- **Four objectives:** First Light, Science First, Lasting Base and Sandbox, each with its own requirements, budget and score weights.
- **Seven design slots, 20 options:**
  - Launch: Falcon 9 or Falcon Heavy, with the number of launches.
  - Surface power: Kilopower-type fission units and solar fields.
  - Radiation shelter: three types.
  - Greenhouse: two sizes.
  - Spare parts.
  - Seven real instruments: Mastcam-Z, SuperCam, PIXL, SHERLOC, MEDA, MOXIE and RAD.
  - Link to Earth: direct antenna or relay orbiter.
  - Every option card says what it is, why it matters, the hardware it is based on, and whether its numbers are NASA / manufacturer values or game values.
- **Live mission dashboard:**
  - **Mass to Mars:** hardware, crew consumables from NASA BVAD, and landing-system mass at Curiosity's ratio in the realistic model, against the launch capacity.
  - **Power cells:** clear sky and in a dust storm.
  - **Budget, data per sol and science value.**
- **Design review:** red lines block the launch, for example "the cargo is 18.8 t but the rockets send only 16.8 t to Mars". Amber lines are risks.
- **Concept maturity meter:** after JPL's Concept Maturity Levels.
- **Known risks before launch:** solar conjunction, dust storms, solar storms and failures.
- **Four roles:** Mission Planner, Systems Engineer, Science Lead, Comms & Budget.
  - Each role has its own systems and its own goal.
  - Each role calls GO in a launch poll, and any change clears the poll.
  - One player can play all four roles.

**Act 2: Fly and live**
- **Transit card:** launches, mass, cruise of about 200 days (NASA) and landing-system mass.
- **A connected Jezero base:** command hub, pressurised corridors, life-support module, science lab, airlock, greenhouses, radiation shelter, ISRU plant, two landing zones, a power yard with cable trays and solar fields. The parts your team designed (fission units, solar fields, shelter shielding, relay antenna and more) appear on the surface.
- **Six phases, 31 sols at Jezero, in first person** on Mars gravity (0.38 g, NASA NSSDC).
  - In each phase you place every power cell your design gave you into AIR, GREENHOUSE and SHELTER.
  - In a dust-storm phase, solar power falls; fission does not.
- **15 dated NASA event cards**, for example:
  - the 2017 solar storm, with Curiosity RAD daily dose 197 → 443 µGy;
  - the 2018 dust storm that ended Opportunity;
  - the 2022 InSight marsquake S1222a;
  - MOXIE, Apollo 13 air filters and solar conjunction.
  - The design changes their cost: shielding lowers the power a radiation event needs, and spares make a repair cheaper. After each choice the card flips to "What really happened?" with the date, instrument, measurement and source.
- **Mission Control delay:** replies arrive one phase late. During solar conjunction no link works, relay or not.
- **Jezero weather from Perseverance MEDA** (PDS archive, labelled ARCHIVE).
- **Today's Sun:** live NASA DONKI space weather labelled `LIVE`, or the 2017 archive labelled `ARCHIVE`.
- **Outcome Shots:** short, skippable cinematics that show each result in the scene.

**Act 3: Mission Debrief**
- **Four score axes before the total:** mission success, science value (only data that reaches Earth counts), efficiency (counts only after a successful lift-off) and decisions.
- **"What broke"** names the design decision behind each problem.
- **Comparisons:** your flights this session, and real NASA missions (Perseverance, Curiosity's landing system, InSight, crewed lander mass).
- **Team debrief:** which role goals held.
- **Redesign** on the same seed, or choose a new objective.

**Everywhere**
- **Source Panel:** every number is NASA or manufacturer data with its source, or a game value labelled as such.
- **Languages and controls:** English and Turkish. Keyboard and mouse.
- **Settings:** simplified or realistic model, Reduce motion, sound.
- **Privacy:** no accounts, no data collection. VR is planned, not in this build.

## Tech

- Unreal Engine 5.8, C++ game module `TouchdownMars`, UMG for the 2D layer, Windows desktop build.
- Git LFS for Unreal assets. Run `git lfs install` once before cloning.
- The `.uproject` file lives at the repository root.

### Repository layout

| Path | What it holds |
| :-- | :-- |
| `Source/TouchdownMars/` | C++: mission design and mission rules (`Model/`), tests (`Tests/`), subsystems and DONKI client (`Systems/`), scene actors, astronaut and camera (`Actors/`), design room (Slate) and UMG widgets (`UI/`) |
| `Data/Source/` | CSV source of truth for every DataTable (mission options, objectives, phases, events, constants, sources, text EN/TR, Jezero weather, DONKI archive, outcome shots) |
| `Content/TouchdownMars/` | Unreal assets: DataTables, widgets, maps (`L_TD_Outpost`, `L_TD_Test`), materials, sounds, NASA 3D meshes |
| `Scripts/` | Python editor scripts (data import, UI and level setup, terrain, art, cameras, balance check, sound synthesis) and `package.ps1` |
| `docs/` | Game design, technical design, level design, production plan, model card, teacher card, project page draft, landing page |

### Build and run

1. Install Unreal Engine 5.8 and Visual Studio 2022 with the "Game development with C++" workload.
2. `git lfs install`, then clone this repository.
3. Open `TouchdownMars.uproject` and let Unreal build the module (or run `Build.bat TouchdownMarsEditor Win64 Development -Project=<path>/TouchdownMars.uproject`).
4. Play `L_TD_Outpost` (the default game map).
5. Package a Windows build: `powershell -ExecutionPolicy Bypass -File Scripts/package.ps1 -Name MyBuild` (close the editor first).

Tests: `UnrealEditor-Cmd.exe TouchdownMars.uproject -ExecCmds="Automation RunTests TD.;Quit" -unattended -nullrhi -testexit="Automation Test Queue Empty"` (mission rules `TD.Model.*` and design rules `TD.Design.*`). Development builds also accept `-TDAutoPlay` (plays the surface mission to the debrief) and the console command `td.AutoPlay`.

Data changes: edit the CSV in `Data/Source/`, then run `Scripts/import_data.py` in the editor (Python console or `exec(open(...).read())`).

### Rebuilding the outpost level

Scripts in this order (inside the editor, Play-in-Editor stopped): `setup_outpost.py` → `setup_terrain.py` → `make_ground_material.py` → `import_nasa_models.py` → `setup_art.py` → `scatter_rocks.py` → `setup_cameras.py` → `setup_props.py` → `setup_tripo.py` → `setup_base.py` (connected base) → `setup_fps.py`.

- The terrain, rock and camera scripts call the free [VibeUE](https://github.com/kevinpbuckley/VibeUE) editor plugin (MIT). Install it into `Plugins/VibeUE/`; it is not part of this repository.
- `setup_terrain.py` needs the NASA Mars Trek elevation file (see `Scripts/dem_to_heightmap.py`); the generated heightmap is in `Data/Terrain/`.
- **Rocks:** the four Megascans rocks are not in this repository (licence). Without them the level opens without rocks. To restore them, add the free Fab listings named below with the Personal licence, import them as glTF (Medium) into `/Game/Fab/Free`, then run `scatter_rocks.py`.

## Data and third-party assets

Full list with URLs, retrieval dates and licences: `Data/Source/sources.csv` and `docs/submission.md` (References).

**NASA data used in the game**
- NASA CCMC DONKI API: live and archived space weather (Today's Sun panel).
- NASA Mars Trek: MRO HiRISE Jezero DEM (1 m/px), a 1 km window near the Perseverance landing site, for the outpost terrain.
- NASA 3D Resources: Habitat Demonstration Unit, Z2 Spacesuit, Space Exploration Vehicle, RASSOR and Tall Dish meshes, with every texture removed (no insignia).
- Mission design numbers: launch vehicle payloads (SpaceX), NASA launch contract values, Kilopower and Fission Surface Power, MMRTG, NSSDC Mars fact sheet, InSight power, MRO relay, Deep Space Network, MSL and Mars 2020 press kits, Perseverance and Curiosity instrument pages, NASA OIG reports, the NASA human Mars landing study (NTRS), NASA BVAD Rev 2.
- NASA PDS archives: Perseverance MEDA (Jezero weather), Curiosity RAD Level 2 (daily dose, September 2017), InSight Marsquake Service catalogue V14 (S1222a).
- NASA/JPL Horizons: Earth–Mars light time. NASA BVAD, OCHMO briefs, Veggie, MOXIE, SAC21 and mission news pages for the numbers and the 14 event cards (see `docs/cards_check.md`).

**Third-party assets and tools**
- Unreal Engine 5.8 (Epic Games, Unreal Engine EULA).
- Quixel Megascans via Fab (Fab Standard License, Personal tier, free; **not redistributed here**): Icelandic Porous Rock, Icelandic Boulder, Desert Western Cluster Rock Medium 05, Icelandic Sand Rock.
- Sound effects: synthesized by `Scripts/make_sfx.py` (Apache 2.0, not AI-generated).
- Tripo AI (AI-generated 3D models, made in the team's Tripo Studio account; listed in `sources.csv` with `bIsAIGenerated=True`): ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut.
- Design-room pictures: Google Gemini illustrations in one locked style (20 option cards, 4 objective cards, the Ares-0 cold open). Each card shows the tag "AI illustration (Gemini)"; listed in `sources.csv` as `GeminiIllustrations` with `bIsAIGenerated=True`.
- Fonts: Barlow Semi Condensed and IBM Plex Mono (SIL Open Font License 1.1, licence files in `Content/TouchdownMars/UI/Fonts`).
- Editor tooling only, not shipped and not in this repository: Epic's Unreal MCP, VibeUE (MIT), a private team fork of the StraySpark Unreal MCP Server.

No NASA logo, flag, insignia or mission patch is used anywhere in the game.

## AI use

The game concept, the design decisions and the choice of NASA sources are the team's. We used AI tools as follows:

- **ChatGPT and Claude** were used to analyse the challenge documents and to set up this repository (`.gitattributes`, `.gitignore`, first README).
- **Anthropic Claude (Claude Code, an AI coding assistant)** wrote the C++ code, the Python editor scripts and the packaging script, built the levels, widgets and DataTables in the Unreal Editor through MCP editor tooling, and drafted the documents, the card research and the English and Turkish game text from the team's research and decisions. Every card fact was checked against its source page (`docs/cards_check.md`).
- **Tripo AI** generated seven 3D models from text prompts written by Claude Code and run in the team's Tripo Studio account: ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut (retopologised and auto-rigged with six preset animations). They are listed in `sources.csv` with `bIsAIGenerated=True`.
- **Google Gemini** generated the 25 design-room illustrations (option cards, objective cards and the cold open of the fictional Ares-0 crew) from prompts written by Claude Code, in one locked style that matches the UI palette. The team checked every image at full size for logos, insignia, flags, mission patches, text and faces and rejected any that had them. Every card carries a visible "AI illustration (Gemini)" tag. Real NASA photos are not used for fictional scenes.
- **Not AI-generated:** the current sounds (team script), the NASA data and meshes, and the Megascans rocks. No AI-generated audio or video is used.

Details: `docs/submission.md`, section "Use of Artificial Intelligence (SI)".

## License

Original content is released under the [Apache License 2.0](LICENSE).

This project is not endorsed by NASA.
