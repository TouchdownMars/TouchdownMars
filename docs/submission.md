# Touchdown Mars: First Light · Space Apps 2026 Project Page (ready to paste)

> Draft for the NASA Space Apps Challenge 2026 project page. Every field below is in English. Lines starting with `TODO:` must be filled before submitting. Do not add names, photos, voices or likenesses of team members.

---

## Project name

Touchdown Mars: First Light

## Team name

Touchdown

## Challenge

Build a Junior Astronaut Mission Trainer

## High-level project summary

Touchdown Mars: First Light is a downloadable 3D mission trainer for children aged 7-12. The child commands the first crew on Mars for 31 sols and splits a few power cells between AIR, GREENHOUSE and SHELTER, one mission phase at a time. Every choice costs something: power given to one system is taken from the others. Real, dated NASA events arrive as cards, such as the September 2017 solar storm measured by Curiosity or the 2018 dust storm in which Opportunity sent its last signal. After each choice the card flips to show what really happened, with its source. Mission Control's advice is correct but arrives late, as messages to Mars do. A Today's Sun panel shows NASA DONKI space weather, labelled LIVE or ARCHIVE. A mission takes about 5-8 minutes, uses only the left mouse button and needs no account.

## Project demo

TODO: 30 s demo video link

## Final project

TODO: download page link

## Project details

### What it does

The child is the Mission Commander of the first crew on Mars. The game opens straight into the outpost, with no menu and no tutorial screen. Mission Control says "Your cargo landed before you. Power is ready." and a ghost hand shows the first move: one power cell into AIR. From then on the child plays six mission phases (Arrival, First walk, Exploration, Resupply and rest, Hard week, Return) from sol 1 to lift-off on sol 31.

Each phase:
1. The child places every power cell into AIR, GREENHOUSE or SHELTER. Each system needs a number of cells; more raises its level, fewer lowers it. GREENHOUSE changes only show in the next phase, because plants grow slowly.
2. An event card arrives face down with a question and two choices. Both choices cost something.
3. The result appears in the scene and in one sentence with one cause ("AIR went down: it got fewer cells than it needed.").
4. The card flips to "What really happened?": a real NASA event with its date, instrument, measurement, a label ("Flown / measured", "NASA plan", "Analog mission on Earth") and its source.
5. "Call home" sends a message to Earth. On Mars the reply arrives one phase later; in the Hard week blackout no reply comes.

If AIR runs out, there is no "Game Over": the crew walks back to the lander and Mission Control asks "Our plan ran out of air in {phase}. Want to change one thing?" One click replays the same mission seed. At lift-off a Mission Report stamps four lines in NASA's priority order (crew first, then base, then science): Crew, Base, Emergency, Bonus, gives a "Minimum" or "Full mission" label, a play-style mission patch and the mission seed. There is no score and no ranking.

Two age bands change the load, not the idea: Cadet (7-9) has 4 cells, 5 phases, two systems to decide per phase and one-sentence cards; Commander (10-12) has 6 cells, 6 phases, three systems and two-sentence cards. English is the default language; Turkish is selectable at any time.

### How it works

- **Mission model (C++).** A pure rules module applies the allocation, events, exposures, delayed messages and report stamps; a world subsystem holds the state and sends events to the UI and scene. A seeded random stream makes the same seed give the same game. 11 automation tests (`TD.Model.*`) cover the allocation table, clamping, "phase 1 cannot be lost", same seed same game, deck guarantees, "no free choice" (every card option costs something), Mission Control delay, delayed greenhouse, radiation exposure, the Cadet band and the lose/report rules.
- **Data-driven content.** Phases, 14 event cards, age bands, constants, sources and 198 English/Turkish text rows live in CSV files imported into Unreal DataTables. A constant marked as NASA data without a source URL fails the import.
- **Model card.** Every number is either NASA data with a source and retrieval date, or a designer value labelled as such in the in-game Source Panel ("Where does this number come from?"). See `docs/model_card.md`.
- **Today's Sun.** At start the game sends one read request each for flares, CMEs and geomagnetic storms of the last 30 days to NASA CCMC DONKI (5 s timeout). On success the panel says `LIVE` with the fetch time in UTC; otherwise it says `ARCHIVE` and shows September 2017 events with their dates. The panel never changes gameplay, so the game is identical offline. It carries the line "DONKI is a research prototype catalogue; it does not give dose values."
- **Balance.** A balance script plays 1,000 seeds per band and policy with the real data. Last run (13 Nov 2026, 14 cards): random play loses 43.9 % of Commander and 15.5 % of Cadet missions, greedy play 0 %, phase 1 loss 0 %. These are computer simulations, not child results.

### Benefits

- Teaches a real constraint of deep-space survival through play: limited power, slow communication, radiation shelter, slow plants.
- Every event is real and dated, and every number shows where it comes from.
- Short (about 5-8 minutes), mouse-only, large targets, every system shown by color, shape, icon and one word (never by red and green alone).
- No accounts, no data collection, works offline; a teacher card (`docs/teacher_card.md`) gives discussion questions for both age bands.

### Intended impact

NASA describes the aim of its K-12 work as to "spark and sustain interest" in STEM, and its K-12 student inventory includes the item "When a project does not work out I can think of other ways to do it". The game is built around that loop: a plan fails, the child changes one thing and flies the same mission again. Ages 7-12 are a band the challenge describes as underserved; earlier NASA games such as Moonbase Alpha aimed at older players.

TODO: tested with children: number of children per age band, what they could explain, the two things we changed (formative playtest, anonymous notes only, no control group).

### Learning design

- Show the rule first, then let them try: in grades 2-5, problem-solving-first instruction showed a small negative effect (g = -0.09; Sinha & Kapur 2021).
- Fewer things to track for younger children: working-memory capacity grows through primary school (Cowan 2016), so Cadet decides two systems per phase.
- No tutorial screen; the first moves teach (Andersen et al. 2012; UNICEF RITEC design deck).
- No points, streaks or badges; a play-style patch is given unannounced; failure is the plan's, never the child's (UNICEF RITEC design deck).

### Tools and languages

- Unreal Engine 5.8 (Windows 64-bit), C++ (game module `TouchdownMars`), Blueprints and UMG for scene and UI wiring.
- Python editor scripts (`Scripts/`) for data import, level and UI setup, terrain, art and cameras, balance simulation and sound synthesis; PowerShell for packaging.
- CSV data (`Data/Source/`) imported into DataTables.
- Unreal HTTP and Json modules for the DONKI request; Enhanced Input; Git with Git LFS.
- Editor tooling (not shipped): Epic's Unreal MCP, VibeUE, a private team fork of the StraySpark Unreal MCP Server, and Claude Code (see "Use of Artificial Intelligence").

## Use of NASA data

| Source | What the game uses it for |
| :-- | :-- |
| NASA CCMC DONKI (live API, `https://ccmc.gsfc.nasa.gov/DONKI/WS/get/{FLR,CME,GST}`) | Today's Sun panel (live): flares, CMEs and geomagnetic storms of the last 30 days, labelled `LIVE` with fetch time |
| NASA CCMC DONKI API, 4-15 Sep 2017 | Today's Sun panel (archive) when offline, labelled `ARCHIVE` |
| NASA Life Support Baseline Values and Assumptions Document (BVAD) Rev 2 | Oxygen use per crew member per day (0.895 kg), AIR card back and Source Panel |
| NASA OCHMO Radiation Protection technical brief | Career radiation limit (600 mSv) |
| NASA/JPL Horizons API (Mars, daily 2017-2032 and 14-16 Nov 2026) | Earth-Mars one-way light time (11.014 min on 14 Nov 2026; shortest 3.2 min, longest 22.25 min) |
| NASA/JPL Horizons API (Moon) | Earth-Moon light time (1.353 s), stored for the optional Moon mode (not built) |
| NASA Veggie fact sheet | Lettuce growing time (33 days); Lettuce card |
| NASA, Second Pepper Harvest Sets Record on Space Station | Pepper growing time (137 days); Pepper card |
| NASA/JPL, Large Solar Storm ... Doubles Radiation Levels on the Martian Surface | Storm2017 card |
| NASA, NASA Watches Mars Light Up During Epic Solar Storm | DoseSpike2024 card |
| George et al. 2024, Space radiation measurements during the Artemis I lunar mission (Nature; PMC and Europe PMC full text) | Mars surface, Moon surface and transit dose rates (0.65, 0.81, 1.58 mSv/day); ArtemisIShelter card (storm shelter vs crew cabin) |
| NASA, Artemis II Moon Mission Daily Agenda; Artemis II Flight Day 8 blog | ArtemisIIShelter card |
| NASA/JPL, NASA's InSight Waits Out Dust Storm | InSight2022 card |
| NASA/JPL, Opportunity Hunkers Down During Dust Storm | DustStorm2018 card |
| Rucker et al., NASA SAC21 Human Mars Architecture (NTRS) and its NTRS record | PowerHookup card (labelled "NASA plan") |
| NASA OCHMO Carbon Dioxide technical brief; NASA Apollo 13 Flight Journal | Apollo13Air card |
| NASA, MOXIE Completes Mars Mission | MOXIE card |
| NASA, What's Mars Solar Conjunction, and Why Does It Matter? | Conjunction card (communication blackout) |
| NASA, Crew Health and Performance Exploration Analog (CHAPEA) | CHAPEA2 card (labelled "Analog mission on Earth") |
| NASA, Let's Ketchup on International Space Station Tomato Research | LostTomatoes card |
| NASA Mars Trek | Outpost terrain: "MRO HiRISE, Jezero DEM, Controlled" (1 m/px), a 1 km window centred at 18.4447 N, 77.4508 E near the Perseverance landing site, 31 m of measured relief; only the small base pad is levelled (design change) |

All of these were retrieved or re-checked on 2026-11-13. Full URLs are under References. Every card fact and its source quote is logged in `docs/cards_check.md`.

## Use of Space Agency Partner data

TODO: CSA food production resource, to be confirmed by Role C

## References

**Data sources (`Data/Source/sources.csv`, retrieved 2026-11-13)**
- BVAD2: NASA Life Support Baseline Values and Assumptions Document (BVAD), Rev 2. https://ntrs.nasa.gov/api/citations/20210024855/downloads/BVAD_2.15.22-final.pdf
- George2024: George et al. 2024, Space radiation measurements during the Artemis I lunar mission, Nature (CC BY 4.0). https://pmc.ncbi.nlm.nih.gov/articles/PMC11446838/
- George2024EPMC: same article, Europe PMC full text. https://www.ebi.ac.uk/europepmc/webservices/rest/PMC11446838/fullTextXML
- OCHMO: NASA Office of the Chief Health and Medical Officer, Radiation Protection technical brief. https://www.nasa.gov/wp-content/uploads/2023/03/radiation-protection-technical-brief-ochmo.pdf
- HorizonsMars: NASA/JPL Horizons API, Mars (499) observed from Earth, 14-16 Nov 2026. https://ssd.jpl.nasa.gov/api/horizons.api?format=json&COMMAND='499'&OBJ_DATA='NO'&MAKE_EPHEM='YES'&EPHEM_TYPE='OBSERVER'&CENTER='500@399'&START_TIME='2026-11-14'&STOP_TIME='2026-11-16'&STEP_SIZE='1%20d'&QUANTITIES='20,21,23'
- HorizonsSeries: NASA/JPL Horizons API, Mars (499) observed from Earth, daily 2017-2032. https://ssd.jpl.nasa.gov/api/horizons.api?format=json&COMMAND='499'&OBJ_DATA='NO'&MAKE_EPHEM='YES'&EPHEM_TYPE='OBSERVER'&CENTER='500@399'&START_TIME='2017-01-01'&STOP_TIME='2032-01-01'&STEP_SIZE='1%20d'&QUANTITIES='20,21,23'&CSV_FORMAT='YES'
- HorizonsMoon: NASA/JPL Horizons API, Moon (301) observed from Earth, 14-15 Nov 2026. https://ssd.jpl.nasa.gov/api/horizons.api?format=json&COMMAND='301'&OBJ_DATA='NO'&MAKE_EPHEM='YES'&EPHEM_TYPE='OBSERVER'&CENTER='500@399'&START_TIME='2026-11-14'&STOP_TIME='2026-11-15'&STEP_SIZE='1%20d'&QUANTITIES='20,21'
- Veggie: NASA Veggie fact sheet. https://www.nasa.gov/wp-content/uploads/2019/04/veggie_fact_sheet_508.pdf
- Pepper2021: NASA, NASA's Second Pepper Harvest Sets Record on Space Station. https://www.nasa.gov/missions/station/nasas-second-pepper-harvest-sets-record-on-space-station
- DONKI: NASA CCMC DONKI (Space Weather Database Of Notifications, Knowledge, Information). https://ccmc.gsfc.nasa.gov/tools/DONKI/
- DONKI2017: NASA CCMC DONKI API, flares, CMEs and storms 4-15 Sep 2017. https://ccmc.gsfc.nasa.gov/DONKI/WS/get/FLR?startDate=2017-09-04&endDate=2017-09-15 (each archived event links its own DONKI page in `Data/Source/donki_archive.csv`)
- Megascans (Quixel via Fab, Fab Standard License, Personal tier, free; not redistributed): Icelandic Porous Rock, Icelandic Boulder, Desert Western Cluster Rock Medium 05, Icelandic Sand Rock (https://www.fab.com/listings/<id>, ids in sources.csv)
- MarsTrek: NASA Mars Trek, MRO HiRISE Jezero DEM, Controlled (JEZ_hirise_soc_006_DTM_MOLAtopography_DeltaGeoid_1m_Eqc_latTs0_lon0_blend40). https://trek.nasa.gov/mars/TrekWS/rest/cat/data/stream?label=JEZ_hirise_soc_006_DTM_MOLAtopography_DeltaGeoid_1m_Eqc_latTs0_lon0_blend40 (retrieved 2026-11-13)
- NASA3DHabitat: NASA 3D Resources, Habitat Demonstration Unit. https://github.com/nasa/NASA-3D-Resources (retrieved 2026-11-13; insignia textures removed)
- NASA3DZ2: NASA 3D Resources, Z2 Spacesuit. https://github.com/nasa/NASA-3D-Resources (retrieved 2026-11-13; insignia textures removed)
- JPL2017Storm: NASA/JPL, Large Solar Storm Sparks Global Aurora and Doubles Radiation Levels on the Martian Surface (29 Sep 2017). https://www.jpl.nasa.gov/news/large-solar-storm-sparks-global-aurora-and-doubles-radiation-levels-on-the-martian-surface/
- NASA2024Storm: NASA, NASA Watches Mars Light Up During Epic Solar Storm (10 Jun 2024). https://www.nasa.gov/missions/nasa-watches-mars-light-up-during-epic-solar-storm/
- ArtemisIIAgenda: NASA, NASA's Artemis II Moon Mission Daily Agenda. https://www.nasa.gov/missions/artemis/nasas-artemis-ii-moon-mission-daily-agenda/
- ArtemisIIFD8: NASA, Artemis II Flight Day 8: Crew Conducts Key Tests on Return to Earth (8 Apr 2026). https://www.nasa.gov/blogs/missions/2026/04/08/artemis-ii-flight-day-8-crew-conducts-key-tests-on-return-to-earth/
- JPLInSight2022: NASA/JPL, NASA's InSight Waits Out Dust Storm (7 Oct 2022). https://www.jpl.nasa.gov/news/nasas-insight-waits-out-dust-storm/
- JPLOpportunity2018: NASA/JPL, Opportunity Hunkers Down During Dust Storm (June 2018, updated 26 Jul 2018). https://www.jpl.nasa.gov/news/opportunity-hunkers-down-during-dust-storm/
- SAC21: Rucker et al., NASA's Strategic Analysis Cycle 2021 (SAC21) Human Mars Architecture, 2022 IEEE Aerospace Conference. https://ntrs.nasa.gov/api/citations/20210026448/downloads/IEEE%202022%20SAC21%20for%20STRIVES%202022-01-06.pdf
- SAC21Record: NASA NTRS citation record 20210026448. https://ntrs.nasa.gov/api/citations/20210026448
- OCHMOCO2: NASA OCHMO, Carbon Dioxide (CO2) Technical Brief OCHMO-TB-004 Rev D. https://www.nasa.gov/wp-content/uploads/2023/12/ochmo-tb-004-carbon-dioxide.pdf
- AFJ13: NASA Apollo 13 Flight Journal, Day 5 part 5: Starting the Battery Charge. https://www.nasa.gov/history/afj/ap13fj/21day5-batterycharge.html
- MOXIE2023: NASA, NASA's Oxygen-Generating Experiment MOXIE Completes Mars Mission (6 Sep 2023). https://www.nasa.gov/missions/mars-2020-perseverance/perseverance-rover/nasas-oxygen-generating-experiment-moxie-completes-mars-mission/
- Conjunction2019: NASA, What's Mars Solar Conjunction, and Why Does It Matter? (23 Aug 2019). https://www.nasa.gov/solar-system/whats-mars-solar-conjunction-and-why-does-it-matter/
- CHAPEA: NASA, Crew Health and Performance Exploration Analog (CHAPEA). https://www.nasa.gov/humans-in-space/chapea
- Tomatoes2023: NASA, NASA: Let's Ketchup on International Space Station Tomato Research (14 Dec 2023). https://www.nasa.gov/missions/station/iss-research/nasa-lets-ketchup-on-international-space-station-tomato-research/

**Learning design (from the team's research notes)**
- Sinha, T. & Kapur, M. (2021). When problem solving followed by instruction works: evidence for productive failure. Review of Educational Research. https://journals.sagepub.com/doi/full/10.3102/00346543211019105
- Cowan, N. (2016). Working memory maturation. https://pmc.ncbi.nlm.nih.gov/articles/PMC4800832
- Andersen, E. et al. (2012). The impact of tutorials on games of varying complexity. CHI 2012. https://grail.cs.washington.edu/projects/game-abtesting/chi2012/chi2012.pdf
- UNICEF and the LEGO Group, RITEC design deck. https://www.unicef.org/childrightsandbusiness/media/1146/file
- NASA K-12 Outcome Assessment evaluation brief. https://www.nasa.gov/wp-content/uploads/2024/05/k-12-oa-evaluation-report-brief-final-508a.pdf
- NASA Space Apps Challenge 2026, Build a Junior Astronaut Mission Trainer. https://www.spaceappschallenge.org/2026/challenges/junior-astronaut-mission-trainer/

**Software and tools**
- UnrealEngine58: Unreal Engine 5.8, Epic Games (Unreal Engine EULA); game engine and default UI font (Roboto, Apache 2.0). https://www.unrealengine.com/
- TeamSFX: Touchdown Mars sound effects, synthesized by `Scripts/make_sfx.py` (Apache 2.0, team-made, not AI-generated); all UI and scene sounds `SFX_TD_*`. https://github.com/TouchdownMars/TouchdownMars/blob/main/Scripts/make_sfx.py
- VibeUE (MIT), editor tooling, not redistributed in our repo. https://github.com/kevinpbuckley/VibeUE
- StraySpark Unreal MCP Server (Fab marketplace product), private team fork used as editor tooling only, not redistributed.
- Claude Code, Anthropic (AI coding assistant). https://claude.com/claude-code
- Megascans and NASA 3D Resources assets are listed by name under Data sources above. No Tripo models, no music and no font other than Roboto are used.

## Use of Artificial Intelligence (SI)

SI was a tool. The idea, the game design and every design decision belong to Team Touchdown. We used two AI tools: **ChatGPT and Claude** to analyse the challenge documents and to set up the repository, and **Anthropic Claude through Claude Code**, an AI coding assistant, for the work listed below.

Where AI was used:
- **Game code:** the C++ module (mission model, rules, tests, DONKI client, scene actors, UI widgets) was written with Claude Code.
- **Editor scripts and editor work:** the Python scripts in `Scripts/` (data import, level and UI setup, terrain, art, rock scatter and camera setup, balance check, sound synthesis, sound import) and the packaging script were written with Claude Code. Claude Code also drove the Unreal Editor through Unreal MCP, VibeUE and our StraySpark MCP fork to build the levels, UMG widget layouts, DataTables, input assets and cameras.
- **Documents:** the design documents, this project text, the README, the model card, the teacher card and the landing page were drafted with Claude Code from the team's research and decisions.
- **Card research and game text:** the event-card research drafts and the English and Turkish card and UI text in `Data/Source/text.csv` were drafted with Claude Code. Every card fact was checked against its source page; the source quotes are logged in `docs/cards_check.md`.

Where AI was not used:
- **Sound:** all sounds are synthesized by a team script (`Scripts/make_sfx.py`, sine waves, noise and envelopes). No AI audio model was used.
- **Images, audio and video:** no AI-generated images, audio or video are used so far. If any are added, images and video will carry a visible "AI-generated" watermark and will be listed here and in `sources.csv`.
- **NASA branding:** no NASA logo, flag, insignia or mission patch is used in the game, the landing page or any generated content.

- **3D models:** no AI-generated 3D models are used (no Tripo). The 3D assets are NASA 3D Resources meshes, Megascans rocks and engine basic shapes.

## Known limits

- **Art.** The outpost stands on NASA Mars Trek Jezero terrain with NASA 3D Resources habitat and Z2 suits (insignia removed) and Megascans rocks; the ground is a procedural regolith material. Some props (lander, cargo, antenna, plant racks, message icon) are engine basic shapes. Crew members are static suits that show their state by color and position, without animation.
- **Child tests pending.** No child has played the game yet; no learning or fun claim is made.
- **Human checks pending.** The team's play-test gates for the prototype, opening minute, cards, Mission Control and Package 1 are not signed off. The agent tested mission flow through the game's functions in the editor, not with a real mouse and keyboard.
- **Not in the current build:** the Warn-Gather-Work emergency ritual, optional science tasks, the prediction and recall questions, Moon mode, crew animations, music and voice.
- **Seed sharing:** the seed is shown on the report and replayed with "Fly again", but cannot be typed in yet.
- **On-screen notes:** "Not endorsed by NASA." and the time-compression note are the first lines of the in-game Source Panel.
- **Time and scale.** 31 sols are played in about 5-8 minutes and Mission Control delay is shown as phases, not minutes; level rules, demands and thresholds are designer values (see `docs/model_card.md`).
- **SAC21 is an analysis option**, labelled "NASA plan" in the game; it is not a committed NASA mission.
- **Windows only.** A 64-bit Windows download; no browser version. The latest test build (Package 4) is 642 MB. TODO: final package size and SHA-256 checksum.

Not endorsed by NASA.
