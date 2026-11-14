# Touchdown Mars: First Light · Space Apps 2026 Project Page (ready to paste)

> Draft for the NASA Space Apps Challenge 2026 project page. Every field below is in English. Lines starting with `TODO:` must be filled before submitting. Do not add names, photos, voices or likenesses of team members.

---

## Project name

Touchdown Mars: First Light

## Team name

Touchdown

## Challenge

Space Mission Design Game

## High-level project summary

Touchdown Mars: First Light is a Mars mission design game for Windows and macOS. One player or a team of up to four, each in a role, designs the cargo mission that lands before a crew of four at Jezero crater. The team chooses the rockets and launches, surface power, radiation shelter, greenhouse, spare parts, science instruments and the link to Earth, under real mass, power, budget and data limits. A live dashboard and a design review show every trade-off and check that the rockets can carry the cargo to Mars. Then the team launches the mission and lives 31 sols in first person, splitting the power their design produced while real, dated NASA events arrive. A debrief scores the mission on success, science, efficiency and decisions, and traces every problem to the design decision that caused it. It also compares the flight with real NASA missions. The team then redesigns and flies the same mission again.

## Project demo

TODO: 30 s demo video link

## Final project

TODO: download page link

## Project details

### What it does

**Act 1: Mission Design Room.** The team picks an objective:
- First Light: bring the crew home and leave the base running.
- Science First: three instruments and two science walks.
- Lasting Base: leave every system healthy for the next crew.
- Sandbox: no budget cap.

Four roles share one screen: Mission Planner (launch), Systems Engineer (power, shelter, greenhouse, spares), Science Lead (instruments) and Comms & Budget (link to Earth, money). Each role changes only its own systems and has its own goal, so the goals pull against each other over the shared mass, power and budget.

Every option card shows its mass, power, cost, data and science value. It also has a "Why it matters" line and says whether its numbers come from NASA or a manufacturer or are game values. Examples:
- A Falcon Heavy sends 16.8 t toward Mars.
- A fission unit gives up to 10 kWe day and night.
- A solar field is light but loses most of its output in a dust storm, as InSight did.
- A relay orbiter returns far more science data but costs 2 t and 250 M$.

The dashboard adds up the cargo's mass against the launch capacity:
- Hardware.
- Crew food, water and oxygen from NASA's BVAD (806 kg for 4 crew × 31 sols).
- In the realistic model, the landing system at Curiosity's ratio of landing system to rover.

The dashboard also shows the power cells, budget, data per sol and science value.

A design review lists what blocks the launch, for example "the cargo is 18.8 t but the rockets send only 16.8 t to Mars", and what is only a risk. A Known-before-launch panel names the risks ahead: solar conjunction, dust storms, solar storms and failures. A JPL Concept Maturity meter climbs from "objective" to "point design". Launch needs all four roles to call GO, and any change clears the poll.

**Act 2: Fly and live.** A transit card shows the launches, the cruise (about 200 days, NASA; Perseverance took 213) and the landing-system mass. The crew lands at Jezero, and the player walks the outpost in first person on Mars gravity (0.38 g).

Each of six phases (sol 1 to sol 31):
1. The player opens the mission console.
2. They split the power cells their design produced between AIR, GREENHOUSE and SHELTER.
3. They answer a dated NASA event card on which both choices cost something.
4. A short, skippable cinematic shows the result in the scene.
5. The card flips to "What really happened?" with the date, instrument, measurement and source.

The design changes the events:
- In a dust-storm phase, solar power falls but fission does not.
- A water-wall shelter needs less power in a radiation storm.
- Spare parts make a repair cheaper.

Mission Control's advice arrives one phase late. During solar conjunction no link works, relay or not.

The helmet shows the Jezero weather of the day from Perseverance's MEDA station (archive). A Today's Sun panel shows live NASA DONKI space weather (LIVE) or the 2017 archive (ARCHIVE).

**Act 3: Mission Debrief.** Four score axes come first, then the total weighted by the objective:
- Mission success.
- Science value: only data that reached Earth counts.
- Efficiency: counted only after a successful lift-off, so an empty rocket cannot win.
- Decisions.

A "What broke" panel names the design decision behind each problem, for example "Only 50 % of the science data reached Earth. Comms & Budget: a relay orbiter returns far more." The debrief compares the flight with the team's other flights this session and with real NASA missions. A team debrief shows which role goals held and asks each role what one thing it would change. "Redesign (same seed)" starts from the flown design.

### How it works

- **Design rules (C++).** A pure module (`UTDDesignRules`) evaluates a design: mass, launch capacity, landing-system mass, power cells in clear sky and in a dust storm, cost, data and science. It produces the review list, the concept maturity level and the role goals, the effects handed to the surface rules, the four-axis score and a shareable design code. Six automation tests (`TD.Design.*`) cover it:
  - The default design is blocked by mass and power.
  - The reference design launches and the poll resets on any change.
  - Solar loses power in dust.
  - The relay carries the data and needs no landing system.
  - Design effects reach the surface.
  - Efficiency needs success.
- **Mission rules (C++).** A second pure module runs the six phases: allocation, events, exposures, delayed messages and report. It reads the design effects. A seeded random stream makes the same seed and the same design give the same mission. Eleven tests (`TD.Model.*`) cover it.
- **Data-driven content.** Everything lives in CSV files imported into Unreal DataTables:
  - 20 design options and 4 objectives.
  - 15 event cards.
  - 164 constants, each with a unit, a source or a designer-value flag, and where in the source it is.
  - 87 sources and about 456 English/Turkish text rows.
  - 31 sols of Jezero weather.

  A constant marked as NASA data without a source URL fails the import.
- **Interface.** The design room and the debrief are built in Slate on a 1920×1080 canvas scaled to any window. The surface uses UMG, a Canvas helmet HUD and an Outcome Shot director for the cinematics.
- **Today's Sun.** One read request to NASA CCMC DONKI at start (5 s timeout). It is labelled LIVE with the fetch time, or ARCHIVE when offline. It never changes gameplay.

### Benefits

- **Trade-offs come before the mission, like real mission design.** Players meet the limits real mission designers work with: launch capacity, landing mass, power through a dust storm, data return and cost.
- **Every decision says why it matters.** Every problem is traced back to the decision that caused it, so players learn why, not only right or wrong.
- **Real hardware and events, with sources.** Real instruments, launch vehicles and NASA events, each with its source. Game values are labelled; archive data is never shown as live.
- **One to four players.** It works for one player or a classroom team, in English and Turkish, offline and with no accounts.

### Intended impact

The challenge asks for a game that teaches how engineering decisions shape mission success and rewards good engineering, efficient resources and collaboration. Touchdown Mars puts the design decision and its consequence in the same 15-minute loop: the team designs, flies, sees what broke and redesigns. Same-screen roles give each player their own systems and goal. This follows cooperative-learning principles (positive interdependence and individual accountability); we do not claim measured learning gains.

TODO: play tests with people outside the team: number of players, what they changed between flights, what we changed.

### Design approach

- **Design → simulate → evaluate → redesign, as in NASA's own process.** NASA's "Design and Develop Science Missions" starts from the need and goals, defines requirements and designs across the launch, space, ground and user segments. JPL's Concept Maturity Levels give the meter; GSFC's Mission Design Lab gives the disciplines behind the four roles.
- **Risks shown before the decision.** The design room lists known risks before launch, and each event card is shown before the power split.
- **Several score axes instead of one number, and no global leaderboard.** Players compare with their own flights and with real missions.
- **Webb "Build It Yourself" as the model for the comparison.** Like NASA's Webb game, the result points to real missions.

### Tools and languages

- Unreal Engine 5.8 (Windows 64-bit and macOS Apple silicon), C++ (game module `TouchdownMars`), Slate and UMG.
- Python editor scripts (`Scripts/`) for data import, level setup, cameras and props; PowerShell and a shell script for packaging.
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

| SpaceX Falcon 9 and Falcon Heavy vehicle pages | Launch capacity to Mars (4,020 kg, 16,800 kg) in the design room |
| NASA launch services contract releases (Europa Clipper, Psyche) | Falcon Heavy launch price in the design room (contract values) |
| NASA Kilopower release 18-031; KRUSTY ground test paper (NTRS); NASA Fission Surface Power page | Fission unit output (up to 10 kWe, at least 10 years; four units for an outpost) |
| NASA NSSDC Mars fact sheet | Solar irradiance on Mars (43 % of Earth), synodic period, distances |
| NASA Photojournal PIA25284; NASA/JPL InSight press kit | Solar output loss from dust (about 5,000 to 500 Wh per sol); InSight cost |
| NASA Mars Relay Network page; NASA/JPL MRO and Mars 2020 telecom pages; NASA Deep Space Network page | Relay orbiter data volume (MRO about 447.5 Mb per day), share of data sent through orbiters (99.9 %), DSN dish sizes |
| NASA Science Perseverance and Curiosity instrument pages; MSL landing press kit | Instrument mass and power (Mastcam-Z, SuperCam, PIXL, SHERLOC, MEDA, MOXIE, RAD); Curiosity's landing-system mass ratio |
| NASA/JPL Mars 2020 press kits; NASA Mars mission timeline | Perseverance mass and cost; cruise time (about 200 days; 213 days for Perseverance); 26-month launch window |
| Cianciolo and Polsgrove 2016, Human Mars Entry, Descent and Landing Architecture Study (NTRS) | Crewed lander payload (20 t) in the debrief comparison |
| NASA OIG reports IG-22-003 and IG-24-001 | SLS cost references (stored for the Moon scenario, not used on Mars) |
| NASA BVAD Rev 2, Table 3-31 | Crew consumables per day (oxygen, potable water, packaged food) for the cargo mass |
| NASA PDS: Perseverance MEDA bundle (doi:10.17189/1522849) | Jezero weather for mission sols 1-31 (Perseverance sols 305-335), labelled ARCHIVE |
| NASA PDS / SPDF CDAWeb: Curiosity RAD Level 2 (doi:10.48322/20qb-4e17) | Daily absorbed dose around the September 2017 solar storm (197.4 and 443.4 µGy per day) |
| InSight Marsquake Service catalogue V14 (doi:10.12686/a21); Kawamura et al. 2023 | Marsquake card S1222a (Mw 4.6 in the catalogue, 4.7 in the paper), 2,715 catalogued events |

All of these were retrieved or re-checked on 2026-11-13 or 2026-11-14. Full URLs are under References. Every card fact and its source quote is logged in `docs/cards_check.md`.

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
- NASA3DSEV, NASA3DRASSOR, NASA3DTallDish: NASA 3D Resources, Space Exploration Vehicle, RASSOR and Tall Dish. https://github.com/nasa/NASA-3D-Resources (retrieved 2026-11-14; textures removed)
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

**Mission design and Mars data sources (`Data/Source/sources.csv`, retrieved 2026-11-14)**
- MegascansPorousRock: Quixel Megascans via Fab: Icelandic Porous Rock. https://www.fab.com/listings/5fc66c1c-7dfd-4e4c-ae11-882c25c1d9e4
- MegascansIcelandicBoulder: Quixel Megascans via Fab: Icelandic Boulder. https://www.fab.com/listings/c6eac3c1-196d-4218-b5f2-c40e9e875cfb
- MegascansDesertCluster: Quixel Megascans via Fab: Desert Western Cluster Rock Medium 05. https://www.fab.com/listings/a5d13b13-f192-4d38-b2ab-78557c9f6e52
- MegascansSandRock: Quixel Megascans via Fab: Icelandic Sand Rock. https://www.fab.com/listings/c95cea58-1e77-4bf1-976e-58f0ab040abd
- NASA3DSEV: NASA 3D Resources, Space Exploration Vehicle. https://github.com/nasa/NASA-3D-Resources/tree/master/3D%20Models/Space%20Exploration%20Vehicle
- NASA3DRASSOR: NASA 3D Resources, Regolith Advanced Surface Systems Operations Robot (RASSOR). https://github.com/nasa/NASA-3D-Resources/tree/master/3D%20Models/Regolith%20Advanced%20Surface%20Systems%20Operations%20Robot%20(RASSOR)
- NASA3DTallDish: NASA 3D Resources, Tall Dish. https://github.com/nasa/NASA-3D-Resources/tree/master/3D%20Models/Tall%20Dish
- NSSDCMars: NASA NSSDC, Mars Fact Sheet. https://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html
- NasaSlsBlock1B: NASA, Space Launch System (SLS) Block 1B reference page. https://www.nasa.gov/reference/sls-space-launch-system-block-1b/
- SpaceXFalconHeavy: SpaceX, Falcon Heavy vehicle page. https://www.spacex.com/vehicles/falcon-heavy/
- SpaceXFalcon9: SpaceX, Falcon 9 vehicle page. https://www.spacex.com/vehicles/falcon-9/
- SpaceXStarship: SpaceX, Starship vehicle page. https://www.spacex.com/vehicles/starship/
- UlaVulcan: United Launch Alliance, Vulcan rocket page. https://www.ulalaunch.com/rockets/vulcan-centaur
- BlueNewGlenn: Blue Origin, New Glenn page. https://www.blueorigin.com/new-glenn
- OigIG22003: NASA OIG, IG-22-003 NASA's Management of the Artemis Missions (Nov 2021). https://oig.nasa.gov/docs/IG-22-003.pdf
- OigIG24001: NASA OIG, IG-24-001 NASA's Transition of the Space Launch System to a Commercial Services Contract (Oct 2023). https://oig.nasa.gov/docs/IG-24-001.pdf
- NasaEuropaClipperLaunch: NASA release 21-098, NASA Awards Launch Services Contract for Europa Clipper Mission. https://www.nasa.gov/news-release/nasa-awards-launch-services-contract-for-europa-clipper-mission/
- NasaPsycheLaunch: NASA, NASA Awards Launch Services Contract for the Psyche Mission. https://www.nasa.gov/news-release/nasa-awards-launch-services-contract-for-the-psyche-mission/
- NasaMarsTimeline: NASA Science, Mars Mission Timeline. https://science.nasa.gov/planetary-science/programs/mars-exploration/mission-timeline/
- JplM2020LaunchMission: NASA/JPL Mars 2020 Perseverance launch press kit, Mission. http://www.jpl.nasa.gov/news/press_kits/mars_2020/launch/mission/
- NssdcMarsFact: NASA NSSDC, Mars Fact Sheet. https://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html
- NasaSpaceComms7: NASA Goddard, Space Communications: 7 Things You Need to Know. https://www.nasa.gov/centers-and-facilities/goddard/space-communications-7-things-you-need-to-know/
- NasaKilopower2018: NASA release 18-031, Demonstration Proves Nuclear Fission System Can Provide Space Exploration Power. https://www.nasa.gov/news-release/demonstration-proves-nuclear-fission-system-can-provide-space-exploration-power/
- Krusty2018: Gibson et al. 2018, The Kilopower Reactor Using Stirling TechnologY (KRUSTY) Nuclear Ground Test Results and Lessons Learned (NTRS 20180005435). https://ntrs.nasa.gov/api/citations/20180005435/downloads/20180005435.pdf
- NasaFsp: NASA, Fission Surface Power project page. https://www.nasa.gov/exploration-systems-development-mission-directorate/fission-surface-power/
- MmrtgFactSheet: NASA Facts NF-2020-05-619-HQ, Multi-Mission Radioisotope Thermoelectric Generator (MMRTG). https://science.nasa.gov/wp-content/uploads/2024/02/mmrtg-factsheet-updated-5-18-20-1.pdf
- InSightPia25284: NASA Photojournal PIA25284, InSight's Power Generation After Landing and Spring 2022. https://science.nasa.gov/photojournal/insights-power-generation-after-landing-and-spring-2022
- JplInSightFacts: NASA/JPL InSight landing press kit, Facts. http://www.jpl.nasa.gov/news/press_kits/insight/landing/facts/
- JplOpportunityRecovery2018: NASA/JPL, Six Things About Opportunity's Recovery Efforts (16 Aug 2018). https://www.jpl.nasa.gov/news/six-things-about-opportunitys-recovery-efforts/
- JplMro2010: NASA/JPL, NASA Mars Orbiter Speeds Past Data Milestone (3 Mar 2010). https://www.jpl.nasa.gov/news/nasa-mars-orbiter-speeds-past-data-milestone/
- NasaMarsRelay: NASA Science, Mars Relay Network. https://science.nasa.gov/mars/mars-relay-network/
- JplM2020Telecom: NASA/JPL Mars 2020 landing press kit, Telecommunications. http://www.jpl.nasa.gov/news/press_kits/mars_2020/landing/mission/spacecraft/telecommunications/
- NasaDsn: NASA, Deep Space Network. https://www.nasa.gov/communicating-with-missions/dsn/
- JplM2020LandingQF: NASA/JPL Mars 2020 Perseverance landing press kit, Quick Facts. https://www.jpl.nasa.gov/news/press_kits/mars_2020/landing/quick_facts/
- MslLandingKit: NASA/JPL, Mars Science Laboratory Landing press kit (July 2012). https://www.jpl.nasa.gov/news/press_kits/MSLLanding.pdf
- Edlas2016: Cianciolo and Polsgrove, Human Mars Entry, Descent, and Landing Architecture Study Overview, AIAA SPACE 2016 (NTRS 20160011571). https://ntrs.nasa.gov/api/citations/20160011571/downloads/20160011571.pdf
- M2020Instruments: NASA Science, Perseverance Science Instruments. https://science.nasa.gov/mission/mars-2020-perseverance/science-instruments/
- MEDA_PDS: Mars 2020 MEDA Bundle (Rodriguez-Manfredi & de la Torre Juarez 2021), NASA PDS, doi:10.17189/1522849. https://atmos.nmsu.edu/PDS/data/PDS4/Mars2020/mars2020_meda/
- MSL_REMS_PDS: MSL REMS Bundle, MODRDR MSL-M-REMS-5-MODRDR-V1.0 (Gomez-Elvira et al. 2012 instrument, doi:10.1007/s11214-012-9921-1). https://atmos.nmsu.edu/PDS/data/PDS4/msl_rems_bundle/
- MSL_REMS_CAB_FEED: Curiosity REMS daily weather feed (Centro de Astrobiologia, outreach use), served by NASA. https://mars.nasa.gov/rss/api/?feed=weather&category=msl&feedtype=json
- MSL_RAD_L2: Hassler D. M. (2026) MSL RAD Level 2 Observation Dosimetry, LET, and Neutral-Particle Data, SPDF, doi:10.48322/20qb-4e17. https://cdaweb.gsfc.nasa.gov/cgi-bin/eval2.cgi?dataset=MSL_RAD_OBS-L2&index=sp_phys
- RAD_SEP2017_PAPER: Zeitlin et al. 2018, Analysis of the Radiation Hazard Observed by RAD on the Surface of Mars During the September 2017 Solar Particle Event, GRL 45(12) 5845-5851, doi:10.1029/2018GL077760. https://doi.org/10.1029/2018GL077760
- RAD_HASSLER2014: Hassler et al. 2014, Mars' Surface Radiation Environment Measured with the MSL Curiosity Rover, Science 343(6169), doi:10.1126/science.1244797. https://doi.org/10.1126/science.1244797
- INSIGHT_APSS_FEED: InSight weather service feed (TWINS/PS daily summaries), NASA - final snapshot sols 675-681. https://mars.nasa.gov/rss/api/?feed=weather&category=insight_temperature&feedtype=json&ver=1.0
- INSIGHT_TWINS_PDS: Rodriguez-Manfredi et al. (2019) InSight APSS TWINS Data Product Bundle, NASA PDS, doi:10.17189/1518950. https://atmos.nmsu.edu/PDS/data/PDS4/InSight/twins_bundle/
- INSIGHT_PS_PDS: Banfield et al. (2019) InSight APSS PS Data Product Bundle, NASA PDS, doi:10.17189/1518939. https://atmos.nmsu.edu/PDS/data/PDS4/InSight/ps_bundle/
- MQS_CAT_V14: InSight Marsquake Service (2023) Mars Seismic Catalogue, InSight Mission; V14 2023-04-01. ETHZ, IPGP, JPL, ICL, Univ. Bristol. doi:10.12686/a21 (copy in PDS InSight SEIS bundle doi:10.17189/1517570). https://pds-geosciences.wustl.edu/insight/urn-nasa-pds-insight_seis/data_derived/mqs_catalog_v14/
- S1222A_PAPER: Kawamura et al. 2023, S1222a - The Largest Marsquake Detected by InSight, GRL 50(5), doi:10.1029/2022GL101543. https://doi.org/10.1029/2022GL101543
- JEZERO_STORM_PAPER: Lemmon et al. 2022, Dust, Sand, and Winds Within an Active Martian Storm in Jezero Crater, GRL 49(17), doi:10.1029/2022GL100126. https://doi.org/10.1029/2022GL100126
- NSSDC_MARS_FACTSHEET: NASA NSSDC Mars Fact Sheet (volumetric mean radius 3389.5 km). https://nssdc.gsfc.nasa.gov/planetary/factsheet/marsfact.html

**Challenge resources and design references**
- NASA Space Apps Challenge 2026, Space Mission Design Game. https://www.spaceappschallenge.org/2026/challenges/space-mission-design-game/
- NASA, Design and Develop Science Missions (2025). https://www.nasa.gov/wp-content/uploads/2025/04/250323-design-and-develop-science-mission.pdf
- Wessen et al., Concept Maturity Levels (JPL, AIAA 2013). https://dataverse.jpl.nasa.gov/dataset.xhtml?persistentId=hdl:2014/44299
- NASA GSFC Mission Design Lab. https://etd.gsfc.nasa.gov/capabilities/integrated-design-center/mission-design-lab/
- NASA Webb, Build It Yourself game. https://science.nasa.gov/mission/webb/game-build-it-yourself/
- NASA CubeSat 101. https://www3.nasa.gov/sites/default/files/atoms/files/nasa_csli_cubesat_101_508.pdf

**Learning design (earlier research, kept for reference)**
- Sinha, T. & Kapur, M. (2021). When problem solving followed by instruction works: evidence for productive failure. Review of Educational Research. https://journals.sagepub.com/doi/full/10.3102/00346543211019105
- Cowan, N. (2016). Working memory maturation. https://pmc.ncbi.nlm.nih.gov/articles/PMC4800832
- Andersen, E. et al. (2012). The impact of tutorials on games of varying complexity. CHI 2012. https://grail.cs.washington.edu/projects/game-abtesting/chi2012/chi2012.pdf
- UNICEF and the LEGO Group, RITEC design deck. https://www.unicef.org/childrightsandbusiness/media/1146/file
- NASA K-12 Outcome Assessment evaluation brief. https://www.nasa.gov/wp-content/uploads/2024/05/k-12-oa-evaluation-report-brief-final-508a.pdf

**Software and tools**
- UnrealEngine58: Unreal Engine 5.8, Epic Games (Unreal Engine EULA); game engine and default UI font (Roboto, Apache 2.0). https://www.unrealengine.com/
- TeamSFX: Touchdown Mars sound effects, synthesized by `Scripts/make_sfx.py` (Apache 2.0, team-made, not AI-generated); all UI and scene sounds `SFX_TD_*`. https://github.com/TouchdownMars/TouchdownMars/blob/main/Scripts/make_sfx.py
- VibeUE (MIT), editor tooling, not redistributed in our repo. https://github.com/kevinpbuckley/VibeUE
- StraySpark Unreal MCP Server (Fab marketplace product), private team fork used as editor tooling only, not redistributed.
- Claude Code, Anthropic (AI coding assistant). https://claude.com/claude-code
- Megascans and NASA 3D Resources assets are listed by name under Data sources above. Tripo AI models are listed under Use of Artificial Intelligence; no music and no font other than Roboto are used.

## Use of Artificial Intelligence (SI)

SI was a tool. The idea, the game design and every design decision belong to Team Touchdown. We used three AI tools: **ChatGPT and Claude** to analyse the challenge documents and to set up the repository, **Anthropic Claude through Claude Code**, an AI coding assistant, for the work listed below, and **Tripo AI** for seven 3D models.

Where AI was used:
- **Game code:** the C++ module (mission design rules, mission rules, tests, design room and debrief screens, DONKI client, scene actors, UI widgets) was written with Claude Code.
- **Research and data:** Claude Code research agents read the challenge resources, GDC talks and developer postmortems, and NASA data archives (MEDA, RAD, InSight). They extracted the sourced numbers and the archive tables; every number in `constants.csv` keeps its source and page.
- **Editor scripts and editor work:** the Python scripts in `Scripts/` (data import, level and UI setup, terrain, art, rock scatter and camera setup, balance check, sound synthesis, sound import) and the packaging script were written with Claude Code. Claude Code also drove the Unreal Editor through Unreal MCP, VibeUE and our StraySpark MCP fork to build the levels, UMG widget layouts, DataTables, input assets and cameras.
- **Documents:** the design documents, this project text, the README, the model card, the teacher card and the landing page were drafted with Claude Code from the team's research and decisions.
- **Card research and game text:** the event-card research drafts and the English and Turkish card and UI text in `Data/Source/text.csv` were drafted with Claude Code. Every card fact was checked against its source page; the source quotes are logged in `docs/cards_check.md`.

Where AI was not used:
- **Sound:** all sounds are synthesized by a team script (`Scripts/make_sfx.py`, sine waves, noise and envelopes). No AI audio model was used.
- **Images (AI):** Google Gemini generated the 25 design-room illustrations (20 option cards, 4 objective cards, the Ares-0 cold open) from prompts written by Claude Code, in one locked style. The team checked each at full size for logos, insignia, flags, mission patches, text and faces. Every card shows a visible "AI illustration (Gemini)" tag; `sources.csv` lists them as `GeminiIllustrations` with `bIsAIGenerated=True`.
- **Audio and video:** no AI-generated audio or video is used.
- **NASA branding:** no NASA logo, flag, insignia or mission patch is used in the game, the landing page or any generated content.

- **3D models (AI):** Tripo AI generated seven models from text prompts written by Claude Code and run in the team's Tripo Studio account: ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut. They are marked `bIsAIGenerated=True` in `sources.csv`. The other 3D assets are NASA 3D Resources meshes, Megascans rocks and engine basic shapes (not AI).

## Known limits

- **Art.** The outpost stands on NASA Mars Trek Jezero terrain with NASA 3D Resources habitat, Z2 suits, Space Exploration Vehicle rover, RASSOR robot and a communication dish (textures removed), Megascans rocks and a Mars-tinted sky; the ground is a procedural regolith material. The ascent vehicle, fission power unit, greenhouse dome, plant rack, cargo crates, shelter hatch and the rigged crew astronaut are Tripo AI models (see Use of Artificial Intelligence). The message icon and the cable are engine basic shapes. Crew members are rigged suits with preset animations (idle, look around, walk, lift, greet, wave goodbye).
- **VR.** Not supported in this build; planned after the hackathon (no headset was available to test it).
- **Play tests pending.** No one outside the team has played the design room yet; no learning claim is made.
- **Game values.** Module masses, prices other than the Falcon Heavy contract, data volumes, science values, the 5 kWe power cell and the budget caps are game values, labelled in the Source Panel. The landing-system mass in the realistic model scales Curiosity's ratio and is a model estimate.
- **Same-screen team play only.** Four roles share one screen; no network play or competing teams yet.
- **Orbits.** Transfer options and orbits are not computed in the game (GMAT is planned).
- **Checks.** The full loop (design room, launch, surface mission, debrief, redesign) was run with real mouse input on the Windows build; 17 automation tests pass. Team play tests are still to come.
- **Not in the current build:** the Warn-Gather-Work emergency ritual, optional science tasks, the prediction and recall questions, Moon mode, crew animations, music and voice.
- **Seed sharing:** the seed is shown on the report and replayed with "Fly again", but cannot be typed in yet.
- **On-screen notes:** "Not endorsed by NASA." and the time-compression note are the first lines of the in-game Source Panel.
- **Time and scale.** 31 sols are played in about 5-8 minutes and Mission Control delay is shown as phases, not minutes; level rules, demands and thresholds are designer values (see `docs/model_card.md`).
- **SAC21 is an analysis option**, labelled "NASA plan" in the game; it is not a committed NASA mission.
- **Platforms.** 64-bit Windows and macOS (Apple silicon) downloads; no browser version. The latest test build (Package 4) is 642 MB. TODO: final package size and SHA-256 checksum.

Not endorsed by NASA.
