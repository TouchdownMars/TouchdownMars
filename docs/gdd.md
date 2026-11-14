# Touchdown Mars: First Light
## Game Design Document (gdd.md)

> **Source of this document:** the 2026 NASA Space Apps challenge page "Space Mission Design Game" (read 14.11.2026) and the team's research reports in `D:\NASA\reports\` ("Space Mission Design Game uyarlaması", "Görev tasarım oyunu GDC dersleri", "Mars oyunu 3D kamera ve gerçekçilik"). Version 1.0 (13.11.2026) was written for the earlier challenge "Build a Junior Astronaut Mission Trainer" (ages 7-12); the team switched challenges on 14.11.2026 and this version replaces it.
> **Markers:** `[DESIGNER VALUE]` = a game number that is not a NASA measurement; it is labelled as such in the game and in `Data/Source/constants.csv`. `MISSING:` = not decided yet.

**Author:** Team Touchdown · **Version:** 2.0 · **Date:** 14.11.2026
**Genre:** space mission design and first-person mission simulation · **Platform:** PC (Windows 64-bit) and macOS (Apple silicon), packaged downloads · **Engine:** Unreal Engine 5.8 · **Players:** 1 to 4 on one screen (four roles)
**Challenge:** NASA Space Apps Challenge 2026, "Space Mission Design Game" (Intermediate, Beginner/Youth; subject Games) · **Audience:** students from beginner to intermediate level, classrooms and families; no age limit

---

## Index
1. Game Design: Pitch, Roles, Gameplay, Mindset, Uniqueness, Design Pillars, Out of Scope
2. Technical: Screens, Controls, Camera, Mechanics
3. Level Design: Philosophy, Themes, Game Flow
4. Development: Classes, Data, AI
5. Graphics
6. Sounds & Music
7. Schedule

---

## 1. Game Design

### Pitch
A crew of four will live 31 sols at Jezero crater on Mars. Before they go, your team designs the cargo mission that lands first: how many rockets, what powers the outpost, how the crew shelters from solar storms, how big the greenhouse is, which spare parts and science instruments fly, and how the data gets home. Every choice has a real mass, power and cost, and the rockets can only send so much to Mars. Then you launch it and live it: you walk the outpost in first person on Mars gravity and split the power your design gave you between AIR, GREENHOUSE and SHELTER while real, dated NASA events arrive. At the end, a debrief scores the mission on four axes, traces every problem back to the design decision that caused it, compares it with real NASA missions, and sends you back to the design room on the same seed to change one thing.

### Roles (players)
The challenge asks for single-player and team play of up to four with roles. The four roles map the 16 disciplines of NASA Goddard's Mission Design Lab into four seats. Each role owns its own systems, has its own goal, and calls GO or NO-GO in the launch poll.

| Role | Owns in the design room | Own goal (shown on its tab) | In the surface mission |
| :-- | :-- | :-- | :-- |
| Mission Planner | Launch vehicle and number of launches | Keep 10 % of launch capacity spare `[DESIGNER VALUE]` | Phase decisions and "Next phase" |
| Systems Engineer | Surface power, radiation shelter, greenhouse, spare parts | At least 5 power cells even in a dust storm | Power-cell allocation |
| Science Lead | Science instruments | 60 science points and the objective's instruments | Science walks on event cards |
| Comms & Budget | Link to Earth, budget | All data reaches Earth; stay within budget | "Call home", data, cost |

- **One player** plays all four roles; the role bar shows whose decision it is.
- **A team** passes the mouse: only the role whose tab is open changes its own systems, which stops one player from taking over (Pandemic designer Matt Leacock: players own their components). The shared mass, power and money pool makes the goals conflict, so the team must negotiate.

### Non-player characters
- **Flight Engineer, Science Officer, Health Officer** (crew, rigged suits with closed visors, no faces): their behaviour shows the state of AIR, GREENHOUSE and SHELTER (working, worried, alarm, shelter, at the lander, celebrate).
- **CAPCOM** (Mission Control): short radio lines; advice is correct but arrives one phase late on Mars and not at all during solar conjunction.

### Gameplay

#### Core Loop
1. **Design** (Act 1): choose an objective; fill every slot; watch mass, power, budget, data and science react; fix the red lines; each role calls GO; launch.
2. **Fly** (transit card): launches, cruise time (NASA: about 200 days), landing with the landing-system mass.
3. **Live** (Act 2, six phases from sol 1 to sol 31): read the phase card and the event card, split the power cells, choose on the card, see the result in the scene (short skippable cinematic), read "What really happened?" with the source.
4. **Debrief** (Act 3): four score axes, "what broke and which decision caused it", your flights, real NASA missions, team debrief.
5. **Redesign** on the same seed, or choose a new objective.

#### Main Mechanics

##### 1. Mission objectives (scenarios)
Four objective cards; each sets the requirements, the score weights and the budget (`objectives.csv`).

| Objective | Requirements | Budget `[DESIGNER VALUE]` | Weights (success / science / efficiency / decisions) |
| :-- | :-- | :-- | :-- |
| First Light | crew home safe, base running, ≥ 1 instrument | 1,500 M$ | 50 / 15 / 20 / 15 |
| Science First | crew home safe, ≥ 3 instruments, ≥ 2 science walks | 1,800 M$ | 30 / 40 / 15 / 15 |
| Lasting Base | crew home safe, every system ≥ Low at lift-off, ≥ 1 instrument | 1,600 M$ | 35 / 10 / 25 / 30 |
| Sandbox | none | no cap | 25 each |

##### 2. Design slots and options
Every option card shows: name, the real hardware it is based on, mass, power out or draw, cost, data, science value, one line "what it is", one line "Why it matters", and whether its numbers are NASA / manufacturer values or game values (`mission_options.csv`, numbers through keys in `constants.csv`).

| Slot (role) | Options | Real numbers used (source in `sources.csv`) |
| :-- | :-- | :-- |
| Launch (Planner) | Falcon 9 (0-6 launches), Falcon Heavy (0-4) | Mars payload 4,020 kg and 16,800 kg (SpaceX); Falcon Heavy price = NASA's Europa Clipper launch contract, about 178 M$ |
| Surface power (Engineer) | Fission unit (0-4), solar field (0-3) | Kilopower up to 10 kWe for ≥ 10 years, four units for an outpost (NASA); Mars sunlight 43 % of Earth (NSSDC); dust: InSight fell from about 5,000 to 500 Wh per sol |
| Radiation shelter (Engineer) | water wall, regolith cover with digging robot, aluminium storm shelter | game values |
| Greenhouse (Engineer) | small, large | game values; lettuce 33 days (NASA Veggie) |
| Spare parts (Engineer, optional) | kit (1 repair), full (2 repairs) | game values |
| Instruments (Science) | Mastcam-Z, SuperCam, PIXL, SHERLOC, MEDA, MOXIE, RAD | mass and power from NASA instrument pages and the MSL press kit; data and science value are game values |
| Link to Earth (Comms) | direct-to-Earth antenna, relay orbiter | MRO relays about 447.5 Mb a day; Perseverance returns 99.9 % of its science data through orbiters (NASA) |

##### 3. Live dashboard and design review
- **Mass to Mars:** hardware + crew consumables + landing system against launch capacity. Consumables come from NASA BVAD Rev 2 Table 3-31 (oxygen 0.895, potable water 3.217, packaged food 2.39 kg per crew member per day) × 4 crew × 31 sols = 806 kg. In the realistic model the landing system scales with the landed mass at Curiosity's ratio (2,401 kg landing system for an 899 kg rover, MSL press kit) and is labelled as a model estimate. A relay orbiter stays in orbit and needs no landing system.
- **Power cells:** (power made − power drawn) / 5 kWe per cell `[DESIGNER VALUE]`, rounded; the dust-storm line shows the cells left when solar output falls to 10 % `[DESIGNER VALUE]` scaled after InSight.
- **Budget, data per sol, science value:** made against allowed.
- **Review list:** red lines block launch (missing slot, over mass, over budget, fewer than 5 cells, too few instruments); amber lines are risks (data that never reaches Earth, a role goal open). Each line names its role.
- **Known before launch:** solar conjunction in the hard week (no link at all, relay or not); dust storms possible in phases 2 and 4; solar storms in phase 3; something will break. Risks are shown before the decision (Into the Breach).
- **Concept maturity (JPL CML):** 1 objective, 2 all systems, 3 fits the limits, 4 point design (all four GO). Launch needs CML 4.

##### 4. GO / NO-GO poll and launch
Each role presses GO only when the design fits the hard limits; any change clears every GO. Launch at CML 4 shows the transit card (launches, mass, cruise about 200 days with Perseverance's 213, landing-system mass, cells) and lands the crew.

##### 5. Power-cell allocation (surface)
Each phase the commander places every cell from the design into AIR, GREENHOUSE and SHELTER. Cells given vs demand changes the level (0 failed, 1 Danger, 2 Low, 3-4 Good): ≥ demand + 1 → +1; = demand → 0; = demand − 1 → −1; ≤ demand − 2 → −2 `[DESIGNER VALUE]`. GREENHOUSE changes one phase later (plants grow slowly). In a dust-storm phase the cells drop to the design's dust-storm cells.

##### 6. Event cards with "What really happened?"
Phases 2-5 draw one card each from a seeded deck of 15 dated NASA events (`events.csv`): the 2017 solar storm, the 2024 dose spike, Artemis I and II shelter, InSight 2022 dust, the 2018 dust storm that ended Opportunity, power hookup, Apollo 13 air filters, MOXIE, the InSight marsquake S1222a, solar conjunction, CHAPEA, lettuce, peppers and lost tomatoes. Both choices cost something. The design changes the cost: better shielding needs fewer cells in a radiation event; a spare-part charge makes one equipment failure one cell cheaper. After the choice the card flips to the dated event, the instrument, the measurement and the source.

##### 7. Delayed Mission Control and solar conjunction
"Call home" once per phase; the reply arrives one phase later (realistic model) or in the same phase (simplified model). In the hard week the Sun stands between Earth and Mars and no link works, relay or not.

##### 8. Radiation
Background dose 0.65 mSv per day on the Mars surface (George et al. 2024). A radiation card with SHELTER below Low, or a "keep working" choice, records a storm exposure. The 2017 card shows Curiosity RAD Level 2 daily absorbed dose: 197.4 µGy on 10 September, 443.4 µGy on 11 September 2017 (absorbed dose, not converted to sieverts).

##### 9. Jezero weather (archive)
The helmet shows the Jezero weather of the phase's first sol from Perseverance MEDA (PDS archive, Perseverance sols 305-335, 28 Dec 2021 - 28 Jan 2022, mapped to mission sols 1-31): air minimum and maximum and mean pressure, labelled ARCHIVE. This window contains the real January 2022 Jezero dust storm. It never claims to be live.

##### 10. Today's Sun (DONKI)
One request to NASA DONKI at start; `LIVE` with the fetch time if it answers, else `ARCHIVE` with the September 2017 events. Information only; play is identical offline.

##### 11. Scoring and debrief
- **Four axes, 0-100, shown before the total** (Zachtronics: several measures beat one number): Mission success (requirements met), Science value (instrument value × share of data that reached Earth, plus science walks), Efficiency (launch capacity used with a 15 % margin as ideal, budget used; counts only after a successful lift-off so an empty rocket cannot score), Decisions (event decisions that kept every system safe with no exposure).
- **Total** = weighted by the objective.
- **What broke:** each problem names the subsystem and the design decision behind it (AIR ran out with N cells; storm exposure and shelter type; dust storm with solar; data short with direct link; empty rocket). Neutral language.
- **Your flights:** this session's flights with design code and total. **Real missions:** Perseverance, Curiosity's landing system, InSight's dust and cost, a crewed lander's 20 t against one Falcon Heavy's 16.8 t.
- **Team debrief:** each role's goal met or open; decisions and science walks; "Each role: what one thing would you change?"
- **Redesign (same seed)** starts from the flown design; **New objective** goes back to the cards.

##### 12. Difficulty: simplified and realistic model
The header toggle sets the model. **Realistic** (default): landing-system mass counted; Mission Control replies one phase late. **Simplified:** no landing-system mass; replies arrive in the same phase; five phases with one system held each phase (the earlier Cadet settings).

##### 13. First flight (onboarding)
The first flight of a session is the tutorial; there is no separate tutorial mode (Civ VI telemetry: few open one). It teaches the **causal logic**, not the buttons. Research: `D:\NASA\research_notes\Görev tasarım oyunu GDC dersleri\onboarding_logic.md` (Factorio FFF #241/#261/#284/#342, Portal commentary, Zachtronics, Into the Breach, Frostpunk, Hodent; Sinha & Kapur 2021, Mayer 2019, Kirschner et al. 2006).

Rules:
- Steps follow the **causal chains**, not the panels: science chain (objective → instrument → data → link), cargo chain (payload → mass → rockets → budget), power chain (power → cells ← dust), then the GO gate.
- **do → see → name:** the player acts, a bar moves, then FLIGHT names the cause in one line. The step strip carries the instruction, so the FLIGHT line is never the only copy of it.
- The player **causes each red line** before it is explained (Factorio: experience not having enough).
- **Predict, then observe:** one tap, never scored, shown next to the real result (data reaching Earth; the dust storm).
- **One cheap failure:** a 6-second storm preview of the player's own design. Solar alone fails it; fission unlocks there.
- **Causal map:** a strip of chips that light up as each link is met; red when that link fails; the same map returns in the debrief.
- **Two fixes with trade-offs** for mass and power (cost, room, cells, mass to carry), instead of one auto-fix.
- **Mission calendar** at the top of the dashboard: dust-storm, solar-storm and no-link sols, telegraphed before launch.

| Step | Link taught | Player does | Ends when |
| :-- | :-- | :-- | :-- |
| Cold open (about 12 s) | design decides survival | watches "Ares-0", a fictional solar-only crew, lose power on a dust sol | "Take the console" (or "Flown before? Skip guidance") |
| 1 | objective → instrument | picks one of two instruments | an instrument is picked |
| 2 | data → link | predicts how much data reaches Earth, sees the real share and the no-link sols | relay added or direct link kept |
| 3 | mass → rockets → budget | books a rocket; one Falcon Heavy is too small (17.7 t for 16.8 t) | the cargo fits |
| 4 | power → cells | adds power to reach 5 cells | 5 clear-sky cells, cargo still fits |
| 5 | dust → cells | predicts the storm, watches the preview fail (1 of 5 cells), adds fission | 5 cells in a dust storm |
| 6 | GO gate | "Poll the room": each console reads its line and value | four GO |
| 7 | launch | launches | transit starts |

- **Locked on flight 1** (card shows the reason): fission (until step 5), large greenhouse, the other two shelters, spares, five of seven instruments, other objectives. Poll and launch stay closed until step 6. "Show me a working design" loads the reference design with three reasons (rockets, power, link) and goes to step 6.
- **Debrief:** "Which decision would you change?" comes **before** the explanation: three decisions, one is the real cause (power units, storm shelter, link, rockets or instruments). Then the trace, the causal map with the path in red, and "Redesign" opens the tab of that decision with its slot pulsing.
- **Flight 2+:** everything unlocked with a NEW tag; the guide speaks only when a line is red (warnings only); the header model toggle returns.
- **Measured:** every step, prediction, fix choice and debrief pick goes to `Saved/Logs/TD_FirstFlight.csv` (time to first GO; target 6 minutes or less; about 2.7 minutes in our own runs).
- `TD.Design.FirstFlightIsSolvable` checks that every step has its red line and its fix with the current data.

##### 14. Base operations (surface crew, tasks, prospecting)
The surface is run like a NASA surface mission, not only a power split. Each phase: **plan** the crew's hours on the base computer, **split** the power cells (TAB console), then **run** the sols: a time-lapse in which the three AI crew walk to the work sites and work while the player watches or does their own tasks on foot. Research: `D:\NASA\research_notes\Mars üs kurulumu ve ekip yapay zekası\` (`game_design_crew_ai.md`: Starfield outposts, RimWorld, Oxygen Not Included, Surviving Mars, The Last of Us Ellie, F.E.A.R., The Sims, Halo 2; `nasa_surface_ops_data.md`: NASA surface-operation numbers).

- **Timeline (NASA HEOMD-415, a 30-sol crewed surface plan that launches on sol 31 like ours):** 8.5 work hours per crew member per sol; no spacewalks on sols 1-3 (the crew re-adapt to gravity and inspect the cargo through robot cameras); first spacewalk on sol 4 after a 2.5 h suit checkout; the rover off-load takes 2.5 h and the remaining deck cargo 1.5 h; a rest sol between spacewalks; never alone outside. Power is already running when the crew lands (robots set up fission in every NASA study), so the crew do not build the reactor.
- **Crew:** the player (Systems Engineer) and three AI crew with roles: Deniz (Mission Planner), Mira (Science Lead), Arda (Comms & Budget). Fictional adults.
- **Base computer (key B):** crew cards with work and spacewalk hours, the task board (open, planned, blocked with the reason), a setup checklist that doubles as onboarding, base status (dust on the arrays, water found, weather mast), the last phase's report, and "Run sols a-b". "Crew choose tasks" lets each AI crew member take the open tasks of their role first (RimWorld-style work priorities; the crew never plays the whole base for the player).
- **Tasks (Data/Source/tasks.csv, 14):** inspect cargo by robot, suit checkout, off-load the rover, off-load the science payload, start the greenhouse, stand up the weather mast (needs MEDA), science traverse to the delta, survey for water-bearing clays, bake water out of clay, pile regolith on the shelter, clean the solar arrays (needs solar), life-support maintenance, restock from the cargo lander, first harvest. Each has phases, a site, crew count, hours (constant), spacewalk or inside, prerequisite, effect and preferred role.
- **Rules:** a spacewalk needs two crew; spacewalk hours per phase = one 4 h spacewalk every second sol after sol 3; no spacewalks in a dust-storm phase; work hours = 8.5 h per sol; a prerequisite planned earlier in the same phase counts.
- **Effects on the mission model:** greenhouse +1, science walk +1 (counts for the objective), clay found (opens extraction), water from clay → AIR +1 (oxygen by electrolysis), regolith berm → radiation relief +1 in solar storms, maintenance → weakest system +1, weather mast → dust on the arrays halved, clean arrays → full cells again. After a dust storm a solar design loses cells to dust until the crew cleans the arrays.
- **Honest geology:** Jezero (18.4°N) has no mapped shallow ice; the crew finds clay. NASA's generic Mars clay case gives 2.7 % water at 300 °C (Kleinhenz 2017); digging uses the RASSOR 2.0 rate (2.7 t per day). Water per run and regolith per berm task are game values derived from these.
- **Crew behaviour (what makes them look smart, from the research):** they walk real paths around the modules (a walkable grid built from the level, A* with straight legs), turn to the work, play the work animation, disappear inside modules for inside work, and show a label over the head: name, what they do, a progress bar, and short spoken lines ("Suit seals checked. On my way."). They stay at their posts and do not teleport; events stop spacewalks.
- **Your own tasks:** tasks planned for the player show a red "YOUR TASK" marker with the distance; at the site, E does the task at once. Tasks left for you are done during the time-lapse.
- **Tests:** `TD.Ops.*` (no spacewalk on arrival, spacewalk takes two, suit check first, rest sol between spacewalks, no spacewalk in dust, work changes the base). Dev flag `-TDOpsDemo` lets the crew plan and run every phase by themselves (with `-TDCam=` for a fixed camera).
- **Deployed parts (Scripts/setup_ops.py):** the base shows what the crew has set up. The rover stays on the cargo lander deck until "Off-load the rover"; payload crates appear by the cargo lander after "Off-load the science payload"; the weather mast stands only after "Stand up the weather mast"; a regolith mound grows on the shelter after the berm task; survey stakes mark the clay site after the survey. Power was set up by robots before the crew landed (every NASA study), so fission and solar are there from sol 1.
- **EVA buddy (never alone outside):** when the player walks more than 18 m from the hub, the nearest idle AI crew member suits up and walks beside the player, a little ahead on the right, walking or hurrying by distance and never teleporting (The Last of Us Ellie rules). The buddy calls out only what the player can see: each site once ("The old delta. Water laid these layers down long ago."), dust lifting, the long walk back. Back near the hub the buddy leaves; a phase run takes them back to work.
- **Rover traverse (5 sols):** in the hard week (sols 21-27, the only phase long enough) two crew drive the pressurized rover out for 5 sols within 20 km (NASA HEOMD-415): two science walks. The rover is gone from the base during the run, then recharges about 2 sols on the fission grid: no rover work (survey, berm, traverse) in the next phase. No traverse in a dust storm (game rule).
- **Fatigue (the rest-sol rule can be broken):** a spacewalk task blocked by the rest-sol rule can still be given ("Give anyway, tired next phase"). The crew card shows NO REST SOL, the report adds a medical line, the crew member works 75 % of the hours next phase (game value), and the debrief lists the broken rule (Frostpunk-style overtime made visible).

#### Win and Lose
- **Lift-off** at the end of phase 6 completes the mission; the score depends on the objective.
- **Loss:** AIR reaches 0. The crew walks to the lander; the debrief explains which design decision and which phase caused it and offers the redesign.

#### Gameplay Walkthrough
1. 0-20 s: cold open: Ares-0, a solar-only crew, loses power in a dust storm. "Take the console". Four objective cards; on the first flight only First Light is open.
2. 20-90 s: first flight steps 1-4: the Science Lead picks an instrument; Comms predicts how much data reaches Earth (39 % with Mastcam-Z on the direct link); the Planner books a Falcon Heavy, sees 17.7 t for 16.8 t and adds a second one; the Engineer adds a second solar field (8 cells).
3. 90-150 s: step 5: "Will your base keep 5 cells in a storm?" The storm preview drops the solar design to 1 cell; fission unlocks; two fission units give 5 cells in a dust storm.
4. Poll the room: four GO, CML 4, Launch. Transit card: two launches, about 200 days, landing systems.
5. Surface, phase 1 Arrival (sols 1-3): walk to the console, place the first cell in AIR. B opens the base computer: Deniz inspects the cargo by robot, Deniz and Arda do the suit checkout, Mira starts the greenhouse. "Run sols 1-3": the crew walk to the airlock and the greenhouse and work.
6. Phases 2-5: dust storm or power hookup, solar storm, equipment failure or marsquake, the hard week with solar conjunction. Between them the crew off-load the rover and the science payload, survey for clay, bake water out of it, pile regolith on the shelter and walk to the delta for science; a dust storm stops all spacewalks.
7. Phase 6 Return: lift-off and debrief: "Only 50 % of the science data reached Earth. Comms & Budget: a relay orbiter returns far more." Redesign on the same seed.

### Mindset
- **Design room:** engineering curiosity and negotiation; every number reacts at once.
- **Launch:** commitment; nothing can be changed after GO.
- **Surface:** responsibility for the design you made; storms you were warned about arrive.
- **Debrief:** understanding why, not shame; the failure belongs to the design and can be fixed.

### Uniqueness
- **NASA's Webb "Build It Yourself"** picks instruments and compares with real missions, with no budget; ours adds mass, power, budget and data limits, a launch compatibility check, four roles and a second act where you live inside the design.
- **Kerbal Space Program** is an open physics sandbox; ours is a short, scored mission-design loop with real dated events and real hardware numbers.
- **Mars Horizon** manages a space agency; ours puts the player on the surface of the mission they designed, in first person.
- Every number has a Source Panel row; game values are labelled; archived data is never shown as live.

### Design Pillars
1. **Every choice costs something.** No free safe option in the design room or on a card (challenge: "every decision has a consequence", "no single perfect solution").
2. **Real, dated and sourced.** NASA and manufacturer numbers with sources; game values labelled; archive data labelled ARCHIVE.
3. **Show the risk before the decision.** Known risks before launch; the event card before the power split (Into the Breach).
4. **Explain why.** Every option has a "Why it matters" line; every problem in the debrief names the decision behind it (challenge: "teach why, not just right or wrong").
5. **Redesign is the progression.** Same seed, one change, compare.

### Out of Scope (this build)
- Networked multiplayer and competing teams (same-screen roles only).
- Orbit mechanics computed in game (GMAT values precomputed later).
- VR (planned after the hackathon).
- Global leaderboards and accounts; nothing leaves the machine except the single DONKI request.
- LLM chatbots.
- Realistic human faces.

---

## 2. Technical

### Screens

#### A. Mission Design Room (start)
- **Objectives:** four objective cards with requirements, budget and score weights.
- **Design:** header (objective, JPL CML meter, model toggle); role bar (four roles, goal state, GO state); slot cards of the active role; dashboard (mass, power, budget, data, science); design review; known risks; GO / NO-GO poll and Launch; design code.
- **Transit:** launch and cruise card (click to land).

#### B. Surface HUD (first person)
Helmet: centre dot, name of what you look at, console hint, Jezero weather line (ARCHIVE). Mission console (Tab or right mouse): phase strip, CAPCOM and "Call home", power panel with cells and the three systems, event card, Today's Sun, Settings, language, mute, Source Panel.

#### C. Pause and Settings
Esc or the gear: resume, restart, new mission, language (EN / TR), sound, screen mode, Reduce motion, Outcome Shots on/off.

#### D. Mission Debrief (end)
Score by axis, what broke, team debrief, your flights, real missions; Redesign (same seed), New objective.

### Controls
> Enhanced Input. Keyboard and mouse, and a gamepad (Xbox layout). The mouse screens (design room, base computer, mission console, debrief) get a gamepad cursor: left stick moves it, A clicks, right stick scrolls, B goes back (`UI/TDGamepadCursor`, built on the engine's analog cursor). While walking, every stick goes to the astronaut.

| Action | Keyboard / Mouse | Gamepad | Notes |
| :-- | :-- | :-- | :-- |
| Move | W A S D or arrows | Left stick | suited walk 140 cm/s, Mars gravity 0.38 |
| Look | Mouse | Right stick | about 140°/s at full stick |
| Hop | Space | A | about 1.4 s in the air on Mars gravity |
| Your task at its site | E | X | |
| Mission console | Tab or right mouse button | Y | cursor on, decisions in the console |
| Base computer | B | View | crew plan and task board |
| Design room, cards, buttons | Left mouse button | Left stick + A | right stick scrolls |
| Back / close | Esc | B | closes the base computer or the console |
| Next phase / run the sols | button, or Space/Enter with the console open | Menu | |
| Skip a cinematic | Left mouse button | A | |

### Camera
- First person: eyes about 1.62 m, FOV 90, no head bob.
- Outcome Shots: short (about 3 s), skippable cinematics show each decision's result in the scene, then return to the eyes; Reduce motion shortens them.
- Design room: 2D screens over the outpost at dusk.

### Mechanics (numbers)
All numbers are in `Data/Source/*.csv`. Key values: cells per 5 kWe; minimum 5 cells; levels 0-4; start AIR 3, GREENHOUSE 2, SHELTER 3 (+1 from water wall or large greenhouse); base demand 2 per system (1 in phase 1); event +1 normal, +2 severe; mass margin goal 10 %; science goal 60; science walk 15 points.

---

## 3. Level Design

### Level Design Philosophy
The outpost is a believable NASA-style surface camp where every system the player designed is visible. Nothing moves unless it means something; light and crew behaviour show state. The design room keeps the outpost visible behind it, so the player sees where the cargo will land.

### Themes
#### Mars Outpost at Jezero (L_TD_Outpost)
- **Mood:** realistic Mars, butterscotch sky, low sun, long shadows (NASA/JPL PIA00917 for sky colour).
- **Objects:** a connected base of about 70 m x 60 m: B1 command hub, pressurised corridors, B2 life support, B3 science lab, B4 airlock with the pressurised rover, greenhouse domes, radiation shelter, ISRU plant, two landing zones with the ascent vehicle and logistics crates, a power yard 60 m out with cable trays, solar fields, dish and UHF relay antenna; three crew members, Megascans rocks, NASA 3D Resources meshes, Tripo props. Parts the team designed appear on the surface (fission units, solar fields, shelter shielding, second greenhouse, relay antenna, MEDA, MOXIE, spares); see level-design.md.
- **Goal:** lift-off with the crew safe and the objective met.

### Game Flow
1. Design room (objective, design, poll, launch, transit).
2. Phase 1 Arrival (sols 1-3): no event.
3. Phase 2 First walk (sols 4-5): power family (hookup, InSight dust, 2018 dust storm).
4. Phase 3 Exploration (sols 7-10): radiation family (2017 storm, 2024 spike, Artemis).
5. Phase 4 Resupply (sols 11-13): equipment and food (Apollo 13 air, MOXIE, marsquake, lettuce, lost tomatoes, dust).
6. Phase 5 Hard week (sols 21-27): solar conjunction, no link.
7. Phase 6 Return (sols 28-31): lift-off, debrief.

---

## 4. Development

### Classes
| Class | Purpose |
| :-- | :-- |
| `UTDDesignRules` (Model/TDMissionDesign) | Pure design rules: evaluate mass, power, money, data; review list; CML; role goals; effects for the surface; score; design code. Unit-tested (`TD.Design.*`). |
| `UTDMissionRules` | Pure surface rules: phases, allocation, events, delay, report; reads the design effects (`FTDRulesConfig::Design`). Unit-tested (`TD.Model.*`). |
| `UTDMissionSubsystem` | Owns design, flights and mission state; design API (`BeginDesign`, `SetDesignPick`, `SetRoleGo`, `LaunchDesign`, `GetScore`); delegates for UI and scene. |
| `STDDesignRoom` (UI/TDDesignRoom) | Slate screens: objectives, design room, transit, debrief, on a 1920×1080 canvas scaled to the window. |
| `ATDPlayerController` | Opens the design room, hides it at landing, opens the debrief. |
| `ATDAstronautCharacter`, `ATDHelmetHUD` | First-person pawn and helmet HUD (Jezero weather line). |
| `UTDShotDirector` | Outcome Shots. |
| `UTDDonkiClient` | One DONKI request. |

### Data
| Data Table | CSV | Contents |
| :-- | :-- | :-- |
| `DT_TD_MissionOptions` | `mission_options.csv` | 20 design options: slot, role, text keys, constant keys, effects, max count, default, code, based-on |
| `DT_TD_Objectives` | `objectives.csv` | 4 objectives: weights, requirements, budget key, sandbox |
| `DT_TD_Constants` | `constants.csv` | every number with unit, source id or designer flag, and where in the source |
| `DT_TD_Sources` | `sources.csv` | every source with URL, retrieval date, licence, use, AI flag |
| `DT_TD_MarsWeather` | `mars_weather.csv` | 31 sols of Jezero weather from Perseverance MEDA (PDS) |
| `DT_TD_Events` | `events.csv` | 15 dated NASA event cards |
| `DT_TD_Phases`, `DT_TD_AgeBands`, `DT_TD_Text`, `DT_TD_DonkiArchive`, `DT_OutcomeShots` | | phases, model settings, EN/TR text, DONKI archive, cinematics |

### AI
No AI opponents and no LLM. Crew behaviour is scripted from system levels.

---

## 5. Graphics
- **Style:** realistic world (Megascans, NASA 3D Resources, Tripo AI props marked in the Source Panel); flat, data-dense mission screens in the design room (dark navy panels, role colours: Planner violet, Engineer orange, Science green, Comms blue; red blocks, amber risks).
- **Lighting:** low sun, Mars sky with Mie-heavy scattering, fixed exposure per area.
- **Needed next:** launch / landing shots; a crew-quarters module interior.

## 6. Sounds & Music
Current SFX are synthesized by the team script. ElevenLabs sounds (wind, footsteps, ambience) are generated and waiting for download and selection; any AI sound will be listed in `sources.csv`. Outside sounds are quieter and muffled (thin air; Perseverance microphones), inside the suit clear.

## 7. Schedule
| Milestone | Date |
| :-- | :-- |
| Challenge switch, analysis and research | 14.11.2026 |
| Design room, design rules, debrief, data sets, tests | 14.11.2026 |
| Docs, team docx, Windows and Mac packages | 14-15.11.2026 night |
| Playtests with outsiders (2-3 people) | 15.11.2026 morning |
| Feature freeze | 15.11.2026 12:30 |
| Demo video, project page, submission | 15.11.2026 before 23:59 |
