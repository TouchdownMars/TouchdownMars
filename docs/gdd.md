# Touchdown Mars: First Light
## Game Design Document (gdd.md)

> **Source of this document:** the team's research reports in `D:\NASA\reports\` (rules, NASA data, child pedagogy, winner patterns) and the 2026 NASA Space Apps guides read on 13.11.2026. There is no filled `GDD_Template.docx`; the reports replace it.
> **Markers:** `PROPOSAL:` = a starting value the team confirms or changes at the Sat 09:00 lock. `MISSING:` = not decided yet; the team must decide. `[DESIGNER VALUE]` = a game number that is not a NASA measurement and is labelled as such in the game.
> **Fill status at the end of this file.**

**Author:** Team Touchdown · **Version:** 1.0 · **Date:** 13.11.2026
**Genre:** turn-based resource-allocation mission simulation for children · **Platform:** PC (Windows 64-bit, packaged download) · **Engine:** Unreal Engine 5.8 · **Players:** single player (a second child may sit next to the first and decide together)
**Challenge:** NASA Space Apps Challenge 2026, "Build a Junior Astronaut Mission Trainer" · **Audience:** children aged 7 to 12

---

## Index
1. Game Design: Pitch, Characters, Gameplay, Mindset, Uniqueness, Design Pillars, Out of Scope
2. Technical: Screens, Controls, Camera, Mechanics
3. Level Design: Philosophy, Themes, Game Flow
4. Development: Abstract Classes, Derived Classes, Data, AI
5. Graphics: Style Attributes, Graphics Needed
6. Sounds & Music: Style Attributes, Sounds Needed, Music Needed
7. Schedule

---

## 1. Game Design

### Pitch
The child is the Mission Commander of the first crew on Mars. A small fission power unit gives the outpost a fixed number of power cells, and the child splits them between AIR, GREENHOUSE and SHELTER, one mission phase at a time, from the first hour after landing to lift-off on sol 31. Real dated NASA events (the September 2017 solar storm, the 2018 dust storm that ended Opportunity, a communication blackout) arrive as cards; after each decision the card flips to show what really happened and where the numbers come from. Mission Control's advice is always correct but arrives late, because a message to Mars takes minutes, so the child learns to plan before the answer comes. Every choice costs something, the crew reacts on screen within a second, and a failed mission restarts with one click and one question: "What one thing would you change?"

### Characters

#### Mission Commander (the player)
- **Role:** player. Not shown on screen; addressed as "you" / "Commander".
- **Look:** none. No avatar, so no gender, skin or body choice is needed.
- **Abilities:** places power cells, answers event cards, presses "Call home", ends a phase.
- **Personality / behaviour:** the child's own.

#### Flight Engineer
- **Role:** NPC crew member, cares for AIR (keeps power and machines running).
- **Look:** realistic surface suit, closed gold visor, no face; blue role stripe and AIR icon on the shoulder.
- **Abilities:** none the child controls; visible behaviour is feedback.
- **Personality / behaviour:** works at the air unit; takes the mask off when AIR is Good; slows down and checks the gauge when AIR is Low; points at the unit when AIR is in Danger; walks to the lander when AIR fails.
- **Name:** MISSING: crew names (team decides; at least one of Engineer or Science Officer is a woman; no real person's name).

#### Science Officer
- **Role:** NPC crew member, cares for GREENHOUSE (grows plants, chooses what to explore).
- **Look:** same suit family, closed visor, green role stripe and leaf icon.
- **Abilities:** none the child controls.
- **Personality / behaviour:** tends plant racks; celebrates when sprouts appear; kneels by wilted racks when GREENHOUSE is Low; goes on the optional science walk.

#### Health Officer
- **Role:** NPC crew member, cares for SHELTER (crew health and the dose badge).
- **Look:** same suit family, closed visor, orange role stripe and shield icon.
- **Abilities:** none the child controls.
- **Personality / behaviour:** checks crew; leads everyone into the shelter during radiation events; holds up the dose badge in the Mission Report.

#### CAPCOM (Mission Control)
- **Role:** NPC voice of Mission Control on Earth.
- **Look:** flat 2D portrait in the top-right corner, headset, friendly; no real person's face.
- **Abilities:** gives advice that is correct but delayed (Mars: arrives one phase later in Commander band).
- **Personality / behaviour:** short spoken-style sentences; praises the plan, not the child ("Moving power before the storm kept everyone safe."); never scolds.

### Gameplay

#### Core Loop
1. A new phase starts; the phase name and sol range appear ("Sol 4-5: First walk").
2. The child splits the power cells between the active systems.
3. An event card arrives face down; first a question, then the child chooses.
4. The result shows in the scene within one second and in one sentence with one reason.
5. The card flips: "What really happened?" with date, instrument, measurement and source.
6. Mission Control's earlier message arrives (late on Mars).
7. The child presses "Next phase". Repeat until lift-off.

#### Main Mechanics

##### 1. Power-cell allocation
The fission unit gives a fixed pool of large power cells. The child drags or clicks a cell into the AIR, GREENHOUSE or SHELTER slot. When a cell lands, the energy line to that system visibly thickens and the others thin. There is never enough for everything once events arrive, so every allocation gives something up. It is fun because the result is instant and visible (lights, fan, plants, crew) and the trade-off is the child's own plan.

##### 2. Event cards with "What really happened?"
Each phase after the first draws one card from a seeded deck. The front asks a question with two choices, each with a cost. After the choice, the card flips to a real dated NASA event with one label: "Flown / measured", "NASA plan", "Analog mission" or "Model estimate". It is fun because the child guesses before seeing the true story, and the story is real.

##### 3. Delayed Mission Control
"Call home" sends a message dot along a line to Earth. On Mars the reply arrives one phase later (Commander band); during the blackout phase no reply comes. The dot shows the delay instead of explaining it. It is fun because the child learns to act before the advice comes, like real crews.

##### 4. Emergency ritual (Warn, Gather, Work)
On radiation cards the Commander band taps three steps in order: Warn (alarm), Gather (crew into shelter), Work (resume only after the all-clear). Cadet band does it in one tap and sees the three steps play.

##### 5. Today's Sun (live DONKI panel)
A small badge opens a panel with today's real space weather from NASA DONKI, labelled `LIVE` with the fetch time, or `ARCHIVE` with its date when offline. It connects the game to the real Sun on the day it is played.

##### 6. Where does this number come from?
A button opens the Source Panel: every number in the game with its source name, URL, retrieval date and whether it is a NASA measurement or a designer value.

#### Win and Lose
- **Win:** reach lift-off at the end of phase 6 (Cadet: phase 5). The Mission Report stamps four lines: Crew (home safe and healthy), Base (left working for the next crew), Emergency (how the storm or failure was handled), Bonus (samples and experiments). Crew + Base = "Minimum mission". All four = "Full mission".
- **Lose:** AIR reaches level 0. The crew walks to the lander and the mission ends early.
- **What happens after losing:** no "Game Over". CAPCOM says "Our plan ran out of air in phase 4. Want to change one thing?" One click restarts the same seed. The first card of the second game asks a recall question with two picture answers ("Last time the storm came. What protected the crew?").

#### Gameplay Walkthrough
1. 0-3 s: no logo, no menu. The Mars outpost appears at dusk, dark. A crew member waves. CAPCOM: "Your cargo landed before you. Power is ready."
2. 3-10 s: CAPCOM: "You are Stay." One power cell pulses; a ghost hand moves it to the AIR slot. Only that move is active. The child clicks: lights come on, the fan spins, the Flight Engineer takes off the mask.
3. 10-20 s: the three systems light up one after another and are named: AIR, GREENHOUSE, SHELTER (icon and one word). A second cell arrives with two valid slots. One sentence: "Power you give to one, the others lose." The child places it; the unchosen line thins.
4. 20-30 s: a "Call home" button appears. The child presses it; the message dot leaves for Earth.
5. 30-45 s: the first event card arrives face down with a sealed "What really happened?" stamp. Question first: "A solar storm is coming. Who stays in the dark?" The child makes a real allocation.
6. 45-60 s: the result shows in the scene and in one line; the card flips (date, instrument, measurement, source). The delayed advice arrives. The crew eats; phase 1 ends.
7. Phase 2 (First walk): a power-hookup failure card: fix it now (costs cells this phase) or lose power later.
8. Phase 3 (Exploration): a solar-storm warning: shelter (science walk cancelled) or keep working (bonus sample, exposure).
9. Phase 4 (Resupply and rest): an equipment failure: repair with what you have.
10. Phase 5 (Hard week): communication blackout on Mars; no advice comes; reduced margin.
11. Phase 6 (Return): load samples, secure the base for the next crew, lift off. Mission Report.

### Mindset
- **Opening minute:** curiosity and control. Something changes on the first click.
- **Phases 2 to 4:** focused tension. The child sees a problem coming (countdown marks) and plans.
- **Event flip:** surprise and pride ("It really happened!").
- **Blackout phase:** responsibility; no one will answer in time.
- **Lift-off and report:** pride in the plan; the crew is safe because of the child's choices.
- **After a loss:** calm, not shame. The failure belongs to the plan, never to the child.

### Uniqueness
- Compared with NASA's own **Moonbase Alpha** (2010, team survival simulation, about 20 minutes, aimed at older players): ours is for 7-12, about 5-8 minutes, one mouse button, and every event is a real dated NASA event with its source.
- Compared with **Kerbal Space Program** (open sandbox physics): ours has no free building and no physics skill barrier; the trade-off decision is the whole game.
- Compared with 2025 Space Apps winners such as **PureFlow** (habitat simulation with live DONKI, Best Mission Concept): we add tested-with-children evidence, a documented model (what is NASA data, what is a designer value), delayed Mission Control as a mechanic, and a teacher card.
- Compared with typical AI-generated hackathon games: realistic 3D Mars built from Mars Trek elevation, no chatbot, and a Source Panel for every number.

### Design Pillars

#### P1. Every choice costs something
There is no cost-free safe option. If a choice has no cost, it is a bug. This is the challenge's own requirement.

#### P2. Real, dated and sourced
Every event and constant is a real NASA measurement, plan or analog mission with a source, or it is labelled as a designer value. Static data is never shown as live.

#### P3. Show the rule first, then let them try
Children aged 7-10 do not learn from failing first (productive failure g = -0.09 in grades 2-5). The opening shows the rule with a ghost hand before the child is tested.

#### P4. One click, one reason
Every action gets an instant confirmation, a scene reaction within one second, and one sentence with one cause ("The greenhouse wilted because it got one cell for two phases.").

#### P5. Crew first, then base, then science
The NASA priority order is printed on every event card and decides the Mission Report order.

### Out of Scope
- Free camera, WASD, first-person view, walkable habitat interior.
- Landing mini-game and rover driving (the rover only moves in the scene in phase 3).
- Cutscenes and an opening cinematic.
- Realistic human faces and lip-synced talking heads (visors stay closed).
- LLM chatbot or any AI that talks to the child.
- Accounts, sign-up, analytics, leaderboard, points, stars, badges.
- Percentage bars and separate quiz screens.
- Choosing the power architecture (fission is the default, per NASA's 2024 decision).
- Multiplayer and online features other than the single DONKI request.

---

## 2. Technical

### Screens

#### A. Main Menu
- **Elements:** none by design. The game opens directly in the outpost scene (opening minute). A small corner group holds Settings (gear), Language (EN/TR) and Mute.
- **Goes to:** In-Game HUD directly.

#### B. In-Game HUD
- **Elements:** phase strip (top-left: six dots, active phase name and sol range); CAPCOM portrait and Earth message line (top-right); Today's Sun badge (bottom-left corner); power panel (bottom third: power cells left, three system slots centre, "Next phase" button right); corner buttons (bottom-right: Mute, Language, "Where does this number come from?"); event card (centre, above the panel, when active); upcoming-event icon; message dot.

#### C. Pause Menu
- **Elements:** opened by Esc or the gear. Resume, Restart same mission, New mission, Age band (Cadet / Commander), Language (EN / TR), Sound on/off, Quit. The game is turn-based, so nothing runs while it is open.

#### D. Game Over / Lose Screen
- **Elements:** the scene stays visible with the crew at the lander. CAPCOM line naming the phase and the system that ran out. Buttons: "Change one thing and try again" (same seed) and "New mission" (new seed). No "Game Over" text.

#### E. Win / Level Complete Screen
- **Elements:** Mission Report with four stamped lines (Crew, Base, Emergency, Bonus), "Minimum mission" or "Full mission" label, an unannounced mission patch name that describes play style ("Careful Planner", "Storm Chaser"), the mission seed (4 digits), from the second game a "last time vs this time" comparison, buttons "Fly again" (same seed) and "New mission". No score, no ranking.

#### F. Settings
- **Elements:** Language (English default, Türkçe), Sound effects on/off, Voice on/off (only if voice ships), Age band, Screen mode (window / fullscreen), Reduce motion on/off.

### Controls
> Enhanced Input. The game is mouse-only by design; keyboard only gives shortcuts.

| Action | Keyboard / Mouse | Gamepad | Input Type | Notes |
| :-- | :-- | :-- | :-- | :-- |
| Move | none | none | none | No character movement in this game. |
| Look / Camera | none | none | none | Fixed camera. |
| Jump | none | none | none | Not used. |
| Interact (place / take cell, press button, flip card) | Left mouse button | not supported | Digital | All play uses only this. Click a cell, then a slot; or drag. |
| Next phase | Left click on button; Space or Enter | not supported | Digital | Same as clicking "Next phase". |
| Close card / panel | Left click on close; Esc | not supported | Digital | Cards never close on their own. |
| Pause / Settings | Esc (when no card is open); gear button | not supported | Digital | |

### Camera
- **View:** fixed diorama camera, three-quarter view from slightly above, looking at the outpost. The scene fills the top two thirds of the screen.
- **Behaviour:** no player control. Allowed camera motion: a slow push-in (about 2 s) during the opening and a gentle 1 s shake-free nudge toward a system when its event resolves; both switch off with "Reduce motion".

### Mechanics

#### 1. Power-cell allocation
- **Rule:** each phase the child places all cells. Each active system has a demand. Comparing cells given with demand changes the system level (0 = failed, 1 = Danger, 2 = Low, 3 and 4 = Good). Levels are shown as a fill with four segments, an icon state and crew behaviour; never as a number.
- **Numbers:** LOCKED 13.11.2026 (team kept the proposal): cells = 6 (Commander) / 4 (Cadet). Start levels AIR 3, GREENHOUSE 2, SHELTER 3. Base demand 2 per system per phase; phase 1 demand 1 (tutorial). Cells given ≥ demand + 1 → level +1; = demand → no change; = demand - 1 → level -1; ≤ demand - 2 → level -2. Levels are clamped to 0..4. All values are `[DESIGNER VALUE]` and are tuned by the balance script and the child tests.
- **Edge cases:** a cell may not be left unplaced ("Next phase" is disabled until all are placed, with the ghost hand showing the last cell after 6-8 s idle). In Cadet band the third system is locked at Good for that phase and needs no cells. GREENHOUSE level changes apply at the start of the next phase (plants grow slowly) and show a countdown mark until then.

#### 2. Event cards
- **Rule:** phases 2 to 5 draw one card each from the seeded deck; every game guarantees at least one power, one radiation and one communication event. A card raises demand on one or more systems or forces a choice between two options, each with a cost. After the choice the card flips to its "What really happened?" side.
- **Numbers:** LOCKED 13.11.2026 (team kept the proposal): a card raises one system's demand by +1 (normal) or +2 (severe) for that phase. Deck size at least 8 cards (target 12). Card text: Cadet one sentence, Commander two sentences.
- **Edge cases:** the same card never appears twice in one game. If a card's facts cannot be verified from its source page on 14 Nov, the card is dropped, not softened. Phase 1 and phase 6 have no event.

#### 3. Mission Control delay
- **Rule:** "Call home" once per phase. The message dot travels the Earth line. Commander band on Mars: the reply arrives at the start of the next phase. Cadet band: it arrives in the same phase. Phase 5 on Mars: blackout, no reply.
- **Numbers:** Earth-Mars one-way light time on 14 Nov 2026: 11.0 minutes; range 3.2 to 22.3 minutes (2017-2031), source JPL Horizons. The game compresses time; the compression is written on screen and in the Science Notes.
- **Edge cases:** if the mission ends before a reply arrives, the Mission Report shows "Message still on its way".

#### 4. Radiation and the dose badge
- **Rule:** background dose on the Mars surface accumulates every sol. On a radiation card, if SHELTER is below level 2 or the child chose "keep working", the crew records one storm exposure.
- **Numbers:** Mars surface 0.65 mSv/day (George et al. 2024, Table 2). Career limit 600 mSv (NASA). Storm exposures are counted, not converted to mSv, because DONKI gives no dose values. Badge visible in Commander band during radiation phases and in the report; Cadet only in the report.
- **Edge cases:** radiation never kills and never "sticks"; when the storm ends the room is clean. The crew never glows.

#### 5. Greenhouse and harvest
- **Rule:** GREENHOUSE at level 3 or more at the start of phase 6 gives a small lettuce harvest (Bonus line). Peppers are shown as "won't grow this mission".
- **Numbers:** red lettuce grew in 33 days in Veggie on the ISS (NASA); peppers took 137 days. The 31-sol mission makes the lettuce come only at the last phase, small: `[DESIGNER VALUE]`.
- **Edge cases:** GREENHOUSE at 0 means no harvest; cargo food still feeds the crew (no mission loss).

#### 6. Lose and restart
- **Rule:** AIR at 0 at the end of any phase ends the mission. Restart uses the same seed.
- **Numbers:** oxygen use 0.895 kg per person per day (NASA BVAD Rev 2, Table 3-31), shown on the AIR card back and in the Source Panel.
- **Edge cases:** phase 1 cannot be lost with the proposed numbers.

#### 7. Today's Sun (DONKI)
- **Rule:** at game start one HTTP GET to `https://ccmc.gsfc.nasa.gov/DONKI/WS/get/FLR` (and `CME`, `GST`) for the last 30 days. Success: panel says `LIVE` with the fetch time in UTC and the number and strongest class of flares. Failure or timeout: panel says `ARCHIVE` and shows the September 2017 events with their dates.
- **Numbers:** timeout 5 s; range at most 60 days (API limit).
- **Edge cases:** the live panel never changes gameplay; it is information only, so the game is identical offline.

#### 8. Age band
- **Rule:** Cadet (7-9) or Commander (10-12) changes the numbers in the table below.
- **Numbers:**

| Setting | Cadet (7-9) | Commander (10-12) |
| :-- | :-- | :-- |
| Phases | 5 (phases 1 and 2 merged) | 6 |
| Power cells | 4 | 6 |
| Systems decided per phase | 2 | 3 |
| Mission Control advice | same or next phase | one phase late on Mars; none in blackout |
| Events | at most one per phase, one cause | one event and one optional science task |
| Emergency ritual | one tap, three steps play | Warn, Gather, Work: three taps |
| Prediction question | none | optional, two picture choices |
| Card text | one sentence | two sentences |
| Technical terms | at most three per game | free, each with a picture and a seven-word meaning on first use |
| Dose badge | report only | radiation phases and report |
| Feedback | full every time | shorter from the second game |

- **Edge cases:** LOCKED 13.11.2026 (team kept the proposal): default Commander; the band switch sits in the corner group and in Settings so an adult can change it before the child plays.

---

## 3. Level Design

### Level Design Philosophy
The scene guides without arrows or text. The ground and sky are muted and still; only the three systems, the power cells and the crew carry saturated color and motion. Nothing moves unless it means something. A pulsing cell and the ghost hand show the first move. Light shows state: a powered system glows in its color, a starving one dims and flickers. Sound confirms every click.

### Themes

#### Theme 1: Mars Outpost (L_TD_Outpost)
- **Mood:** realistic Mars, late afternoon light, butterscotch sky, long shadows; dusk and dark at the opening, light coming on with the first click; dust haze during the dust-storm card.
- **Objects:** habitat module (centre), greenhouse dome (left), radiation shelter (right), small fission power unit (back, at a distance), lander (back), rover (moves only in phase 3), three crew members (front), cargo crates, plant racks, antenna pointing at Earth.
- **Challenges:** event cards (power hookup failure, solar storm, dust storm, equipment failure, communication blackout).
- **Goal:** reach lift-off with the crew safe and the base working.

#### Theme 2: Moon Outpost (L_TD_MoonOutpost, optional, cut first if time runs short)
- **Mood:** black sky, hard white sunlight, grey regolith.
- **Objects:** the same three systems and crew with lunar dressing.
- **Challenges:** lunar night instead of blackout; Mission Control replies almost at once (1.3 s light time), which shows the Mars delay by contrast.
- **Goal:** the same as Mars.

### Game Flow
1. Phase 1 Arrival (sol 1-3): stay decision, power hookup, air and water check, call home, first seeds. No event. Nearly impossible to lose.
2. Phase 2 First walk (sol 4-5): short walk, unload cargo. Event: power hookup failure.
3. Phase 3 Exploration (sol 7-10): stay within consumables, return in time. Event: solar-storm warning. Optional science: ice search or weather station.
4. Phase 4 Resupply and rest (sol 11-13): charge, resupply, trash, day off. Event: equipment failure. Optional: plant experiment.
5. Phase 5 Hard week (sol 21-27; `[DESIGNER VALUE]` set in Phase 1 because the sol range was missing; team may change it in `phases.csv`): protect the crew with a reduced margin. Mars: communication blackout; Moon: lunar night.
6. Phase 6 Return (sol 28-31): load samples, secure the base for the next crew, lift off. Mission Report.

---

## 4. Development

### Abstract Classes

| Class | Parent | Purpose |
| :-- | :-- | :-- |
| `UTDMissionSubsystem` | `UWorldSubsystem` | Owns the mission state, seed, deck, delay queue; exposes functions and delegates to UI and scene. |
| `UTDMissionRules` | `UObject` | Pure rules: applies an allocation and an event to a state and returns the new state. No engine dependencies; unit-tested. |
| `UTDDonkiClient` | `UObject` | One HTTP request per session to DONKI; returns a snapshot or the archive. |
| `ATDSystemModule` | `AActor` | Base for the three systems in the scene; reacts to level changes. |
| `ATDCrewMember` | `AActor` | Base for crew members; plays behaviour by system state. |
| `ATDGameMode` | `AGameModeBase` | Sets the camera, input mode and starts the mission. |
| `ATDPlayerController` | `APlayerController` | Shows the mouse cursor, routes clicks and shortcuts. |

### Derived Classes

| Blueprint | Derives from | What it adds |
| :-- | :-- | :-- |
| `BP_TD_AirUnit` | `ATDSystemModule` | Fan, blue light, mask behaviour trigger. |
| `BP_TD_Greenhouse` | `ATDSystemModule` | Plant racks with growth stages, green light. |
| `BP_TD_Shelter` | `ATDSystemModule` | Door, orange light, shelter-in animation trigger. |
| `BP_TD_FlightEngineer` | `ATDCrewMember` | AIR behaviours. |
| `BP_TD_ScienceOfficer` | `ATDCrewMember` | GREENHOUSE and science-walk behaviours. |
| `BP_TD_HealthOfficer` | `ATDCrewMember` | SHELTER and dose-badge behaviours. |
| `BP_TD_PowerUnit` | `AActor` | Energy lines to the three systems (thickness by cells). |
| `BP_TD_Rover` | `AActor` | Moves along a spline in phase 3 only. |
| `BP_TD_CameraRig` | `AActor` | Fixed camera, opening push-in. |
| `GM_TD` | `ATDGameMode` | Default pawn none, HUD widget class. |
| `PC_TD` | `ATDPlayerController` | Input mapping context. |

### Data

| Data Table | Rows (examples) | Fields |
| :-- | :-- | :-- |
| `DT_TD_Phases` | Arrival, FirstWalk, Exploration, Resupply, HardWeek, Return | PhaseId, SolRange, NameKey, Demand per system, EventFamily, OptionalScienceKey, SourceIds |
| `DT_TD_Events` | Storm2017, DoseSpike2024, DustStorm2018, InSight2022, Apollo13Air, ArtemisIIShelter, Conjunction, Lettuce, Pepper, LostTomatoes, MOXIE, PowerHookup, CHAPEA2 | EventId, Family, Label, AllowedPhases, Band, QuestionKey, ChoiceA/B keys and effects, RevealKey, EventDate, Instrument, Measurement, SourceIds |
| `DT_TD_AgeBands` | Cadet, Commander | Cells, Phases, SystemsDecided, AdviceDelayPhases, CardSentences, TermLimit, Ritual taps |
| `DT_TD_Constants` | O2PerPersonDay, MarsSurfaceDose, MoonSurfaceDose, TransitDose, CareerLimit, LightTimeNow, LightTimeMin, LightTimeMax, LettuceDays, PepperDays, MOXIETotal, MOXIERate | Key, Value, Unit, SourceId, IsDesignerValue |
| `DT_TD_Sources` | BVAD2, George2024, Horizons, Veggie, DONKI, MarsTrek, CSAFood, every asset pack and model | SourceId, Name, URL, RetrievedOn, Licence, UsedFor |
| `DT_TD_Text` | every UI string and card text | Key, EN, TR |
| `DT_TD_DonkiArchive` | September 2017 flare and storm events | EventType, ActivityId, Time, Class, SourceURL |

### AI
There are no enemies and no AI opponents. Crew members are scripted: each one listens to the level of its system and plays one of five behaviours (Working, Worried, Alarm, Shelter, AtLander) plus Celebrate at lift-off. No perception, no navigation beyond fixed spline paths. No LLM.

---

## 5. Graphics

### Style Attributes
- **Art style:** realistic world, schematic decision layer. Terrain, sky, light, habitat and hardware look real; the cells, slots, gauges, cards and report are flat 2D, large and color-coded. Crew in realistic suits with closed visors.
- **Color palette:** world in muted Mars tones (butterscotch sky, rust and ochre ground); saturated color only on systems and UI: AIR blue, GREENHOUSE green, SHELTER orange, power cells warm yellow. Hex values in `design.md`.
- **References:** NASA Mars 2020 / Perseverance imagery and Mars Trek terrain for the world; NASA surface habitat concept art for hardware. MISSING: reference games chosen by the team.
- **Lighting:** warm low sun; dark at the opening; each system lit by its own colored light that brightens with its level. PROPOSAL (kept): Lumen on if the weakest laptop holds 30 fps, else baked lighting.

### Graphics Needed

#### Characters
| Asset | Description | Animations |
| :-- | :-- | :-- |
| `SK_TD_CrewSuit` | Surface suit with closed visor, one mesh, role stripe and icon by material instance (blue, green, orange). Tripo-generated, rigged to a humanoid skeleton. | Idle, Working, Worried, Point, Walk (to shelter and lander), Celebrate, Wave, Mask off (AIR good). |
| `T_TD_Capcom` | 2D portrait for Mission Control, headset, neutral friendly. | Two frames: talking, listening. |

#### Environment and Props
| Asset | Description | Where used |
| :-- | :-- | :-- |
| Mars terrain | Landscape from Mars Trek elevation data of the outpost site, Megascans surface materials | Whole scene |
| `SM_TD_Habitat` | Habitat module | Centre |
| `SM_TD_GreenhouseDome` | Inflatable greenhouse with plant racks | Left |
| `SM_TD_Shelter` | Radiation shelter with door | Right |
| `SM_TD_FissionUnit` | Small surface fission power unit | Back |
| Lander | NASA 3D Resources model if a suitable lander exists; else Tripo generic lander without NASA markings | Back |
| Rover | NASA 3D Resources rover model | Phase 3 path |
| `SM_TD_PlantRack_Stage0..3` | Plant racks in four growth stages | Greenhouse |
| `SM_TD_CargoCrate` | Cargo crates | Around habitat |
| `SM_TD_Antenna` | Dish pointing at Earth | Habitat roof |
| Rocks | Megascans rocks | Foreground dressing |

#### Visual Effects (VFX)
| Effect | When it plays |
| :-- | :-- |
| `NS_TD_EnergyFlow` | Along each energy line; speed and width by cells given |
| `NS_TD_DustHaze` | Dust-storm card phase |
| `NS_TD_StormAlert` | Radiation card: light sweep on the shelter, no glowing crew |
| `NS_TD_Sprout` | Greenhouse level rises |
| `NS_TD_LiftOff` | Phase 6 end |

#### UI Art
| Element | Description |
| :-- | :-- |
| Power cell | Large rounded cell, warm yellow, pulse animation when it is the next move |
| System slot | Big slot with icon, word and four-segment fill |
| System icons | AIR: circle with fan blades; GREENHOUSE: leaf; SHELTER: shield |
| Event card | Front with question and two choices; back with date, instrument, measurement, label, source line |
| Card labels | "Flown / measured", "NASA plan", "Analog mission", "Model estimate" badges |
| Phase strip | Six dots, active dot larger with phase name and sol range |
| Message dot and Earth line | Dot travelling along a line to a small Earth icon |
| Ghost hand | Semi-transparent hand pointer |
| Mission Report | Four stamped lines, patch, seed |
| Mission patch | Play-style patch art without NASA branding |

---

## 6. Sounds & Music

### Style Attributes
Realistic and calm, never scary. Soft mechanical hums for systems, clear short UI confirmations, Mission Control radio texture. Alarms are firm but not shrill. Every click has a sound; nothing responds silently.

### Sounds Needed
| Sound | When it plays |
| :-- | :-- |
| `SFX_TD_CellPick` | Cell picked up |
| `SFX_TD_CellPlace` | Cell placed in a slot |
| `SFX_TD_SystemUp` | A system level rises |
| `SFX_TD_SystemDown` | A system level falls |
| `SFX_TD_FanLoop` | AIR running (pitch by level) |
| `SFX_TD_CardArrive` | Event card arrives |
| `SFX_TD_CardFlip` | Card flips to "What really happened?" |
| `SFX_TD_RadioIn` | CAPCOM message arrives |
| `SFX_TD_MessageSend` | "Call home" pressed |
| `SFX_TD_Alarm` | Warn step of the emergency ritual |
| `SFX_TD_DoorClose` | Crew enters shelter |
| `SFX_TD_Wind` | Dust storm |
| `SFX_TD_LiftOff` | Lift-off |
| `SFX_TD_Stamp` | Each Mission Report line stamped |
| `SFX_UI_Hover`, `SFX_UI_Confirm`, `SFX_UI_Back`, `SFX_UI_Locked` | UI tokens in `design.md` |

### Music Needed
| Track | Where / when | Mood |
| :-- | :-- | :-- |
| Outpost theme | During play, low under the systems | Calm, curious |
| Mission Report sting | When the report opens | Proud |
| MISSING: music source and licence | All tracks | Music is in the cut list; cut before voice is restored |

---

## 7. Schedule

| Milestone | What is done | Date |
| :-- | :-- | :-- |
| Lock and setup | Three systems, cell rule and six phases locked; empty project, public repo with LICENSE, empty scene packaged and uploaded; project page draft saved | 14.11.2026 09:00-10:30 |
| Prototype | Mission model with seeded RNG and tests; greybox scene, fixed camera, power panel | 14.11.2026 14:30 |
| First playable (Package 1) | Six-phase loop, delay queue, restart, opening minute, eight sourced cards | 14.11.2026 18:30 |
| Child test 1 | Two children aged 7-9 | 14.11.2026 19:00-20:30 |
| Package 2 | Card flip, Mission Report, DONKI panel with archive, Cadet settings | 15.11.2026 ~01:00 |
| Art and audio pass | Mars Trek terrain, Megascans, NASA and Tripo models, light, SFX | 15.11.2026 07:00-09:30 |
| Package 3 and child test 2 | Three children aged 10-12 | 15.11.2026 09:30-11:00 |
| Feature freeze | Only fixes after this | 15.11.2026 12:30 |
| Final | Final package, 30 s demo, full gameplay video, landing page, project page, submit | 15.11.2026 15:00 → jury |
| Deadline | Last edit and resubmit allowed | 15.11.2026 23:59 |

---

## Fill status
| Section | Status |
| :-- | :-- |
| 1 Pitch | FILLED |
| 1 Characters | FILLED (crew names MISSING) |
| 1 Gameplay, Core Loop, Main Mechanics | FILLED |
| 1 Win and Lose, Walkthrough | FILLED |
| 1 Mindset, Uniqueness, Pillars, Out of Scope | FILLED |
| 2 Screens, Controls, Camera | FILLED |
| 2 Mechanics | FILLED; numbers and age-band selection LOCKED 13.11.2026 (proposal kept) |
| 3 Level Design | FILLED |
| 4 Development | FILLED |
| 5 Graphics | FILLED (reference games MISSING) |
| 6 Sounds & Music | FILLED (music source MISSING) |
| 7 Schedule | FILLED |
