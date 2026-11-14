# Touchdown Mars

A 3D mission trainer game for children aged 7–12, in development for the **NASA Space Apps Challenge 2026** (challenge: *Build a Junior Astronaut Mission Trainer*) by **Team Touchdown**, İstanbul.

The player runs a small Mars outpost and splits a limited supply of power between three systems: **AIR** (life support), **GREENHOUSE** (food production) and **SHELTER** (protection and indoor work). Protecting one system reduces what the others can do; there is no free, safe option.

## Status

Work in progress during the hackathon (14–15 November 2026). A packaged Windows build runs end to end. The team's own play tests and the tests with children are still to come, so no learning or fun claim is made yet.

## What is in the game now

- **Six phases, 31 sols** at Jezero Crater (five phases for the Cadet band, ages 7–9). Each phase: place every power cell, answer one event card, see the result, then the card flips to "What really happened?" with the real date, instrument, measurement and source.
- **14 event cards**, each based on a real NASA event, plan or analog mission (radiation storms, dust storms, equipment failures, communication blackouts, plant growth).
- **Mission Control delay:** "Call home" gets its answer one phase later; in the blackout week no answer comes.
- **Today's Sun:** live NASA DONKI space weather (flares, CMEs, geomagnetic storms of the last 30 days), labelled `LIVE`; offline it shows a September 2017 archive labelled `ARCHIVE`.
- **Source Panel** ("Where does this number come from?"): every number is either NASA data with its source or a designer value labelled as such.
- **Camera:** one fixed three-quarter camera for every decision; short (about 3 s), skippable Outcome Shots play only when a decision's result is shown.
- **English and Turkish**, mouse-only play, Reduce motion and sound settings, no accounts and no data collection.

## Tech

- Unreal Engine 5.8, C++ game module `TouchdownMars`, UMG for the 2D layer, Windows desktop build.
- Git LFS for Unreal assets. Run `git lfs install` once before cloning.
- The `.uproject` file lives at the repository root.

### Repository layout

| Path | What it holds |
| :-- | :-- |
| `Source/TouchdownMars/` | C++: mission rules and tests (`Model/`), subsystems and DONKI client (`Systems/`), scene actors and camera (`Actors/`), UI widgets (`UI/`) |
| `Data/Source/` | CSV source of truth for every DataTable (phases, events, constants, sources, text EN/TR, DONKI archive, outcome shots) |
| `Content/TouchdownMars/` | Unreal assets: DataTables, widgets, maps (`L_TD_Outpost`, `L_TD_Test`), materials, sounds, NASA 3D meshes |
| `Scripts/` | Python editor scripts (data import, UI and level setup, terrain, art, cameras, balance check, sound synthesis) and `package.ps1` |
| `docs/` | Game design, technical design, level design, production plan, model card, teacher card, project page draft, landing page |

### Build and run

1. Install Unreal Engine 5.8 and Visual Studio 2022 with the "Game development with C++" workload.
2. `git lfs install`, then clone this repository.
3. Open `TouchdownMars.uproject` and let Unreal build the module (or run `Build.bat TouchdownMarsEditor Win64 Development -Project=<path>/TouchdownMars.uproject`).
4. Play `L_TD_Outpost` (the default game map).
5. Package a Windows build: `powershell -ExecutionPolicy Bypass -File Scripts/package.ps1 -Name MyBuild` (close the editor first).

Model tests: `UnrealEditor-Cmd.exe TouchdownMars.uproject -ExecCmds="Automation RunTests TD.Model;Quit" -unattended -nullrhi -testexit="Automation Test Queue Empty"`.

Data changes: edit the CSV in `Data/Source/`, then run `Scripts/import_data.py` in the editor (Python console or `exec(open(...).read())`).

### Rebuilding the outpost level

Scripts in this order (inside the editor, Play-in-Editor stopped): `setup_outpost.py` → `setup_terrain.py` → `make_ground_material.py` → `import_nasa_models.py` → `setup_art.py` → `scatter_rocks.py` → `setup_cameras.py`.

- The terrain, rock and camera scripts call the free [VibeUE](https://github.com/kevinpbuckley/VibeUE) editor plugin (MIT). Install it into `Plugins/VibeUE/`; it is not part of this repository.
- `setup_terrain.py` needs the NASA Mars Trek elevation file (see `Scripts/dem_to_heightmap.py`); the generated heightmap is in `Data/Terrain/`.
- **Rocks:** the four Megascans rocks are not in this repository (licence). Without them the level opens without rocks. To restore them, add the free Fab listings named below with the Personal licence, import them as glTF (Medium) into `/Game/Fab/Free`, then run `scatter_rocks.py`.

## Data and third-party assets

Full list with URLs, retrieval dates and licences: `Data/Source/sources.csv` and `docs/submission.md` (References).

**NASA data used in the game**
- NASA CCMC DONKI API: live and archived space weather (Today's Sun panel).
- NASA Mars Trek: MRO HiRISE Jezero DEM (1 m/px), a 1 km window near the Perseverance landing site, for the outpost terrain.
- NASA 3D Resources: Habitat Demonstration Unit, Z2 Spacesuit, Space Exploration Vehicle, RASSOR and Tall Dish meshes, with every texture removed (no insignia).
- NASA/JPL Horizons: Earth–Mars light time. NASA BVAD, OCHMO briefs, Veggie, MOXIE, SAC21 and mission news pages for the numbers and the 14 event cards (see `docs/cards_check.md`).

**Third-party assets and tools**
- Unreal Engine 5.8 (Epic Games, Unreal Engine EULA).
- Quixel Megascans via Fab (Fab Standard License, Personal tier, free; **not redistributed here**): Icelandic Porous Rock, Icelandic Boulder, Desert Western Cluster Rock Medium 05, Icelandic Sand Rock.
- Sound effects: synthesized by `Scripts/make_sfx.py` (Apache 2.0, not AI-generated).
- Editor tooling only, not shipped and not in this repository: Epic's Unreal MCP, VibeUE (MIT), a private team fork of the StraySpark Unreal MCP Server.

No NASA logo, flag, insignia or mission patch is used anywhere in the game.

## AI use

The game concept, the design decisions and the choice of NASA sources are the team's. We used AI tools as follows:

- **ChatGPT and Claude** were used to analyse the challenge documents and to set up this repository (`.gitattributes`, `.gitignore`, first README).
- **Anthropic Claude (Claude Code, an AI coding assistant)** wrote the C++ code, the Python editor scripts and the packaging script, built the levels, widgets and DataTables in the Unreal Editor through MCP editor tooling, and drafted the documents, the card research and the English and Turkish game text from the team's research and decisions. Every card fact was checked against its source page (`docs/cards_check.md`).
- **Not AI-generated:** the sounds (team script), the NASA data and meshes, and the Megascans rocks. No AI-generated images, audio, video or 3D models are used. If any are added, they will be listed here and marked visibly.

Details: `docs/submission.md`, section "Use of Artificial Intelligence (SI)".

## License

Original content is released under the [Apache License 2.0](LICENSE).

This project is not endorsed by NASA.
