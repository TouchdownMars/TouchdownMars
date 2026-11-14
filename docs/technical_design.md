# Touchdown Mars: First Light · Technical Design Document (technical_design.md)

> **Canonical design source:** `gdd.md`. This document turns the GDD into a concrete Blueprint / C++ architecture. **Where this document and the GDD disagree, the GDD wins** and this document must be corrected.
> **Engine target:** Unreal Engine 5.8 (Windows 64-bit).
> **Companion documents:** `gdd.md` (what the game is) and `level-design.md` (where things are).

**Status legend:** `✅ Built` · `🟡 Partial` · `🔲 Not started` · `♻️ Built but must change`
All systems below are `🔲 Not started`: no project work exists before 14.11.2026 09:00.

---

## 0. Meta & Conventions

### 0.1 How to read this document
Every system section uses the same structure: **GDD basis → Architecture → Class spec → Current state → Required work.**

### 0.2 Naming Conventions
- **C++ classes:** `A` (Actor), `U` (UObject / Component), `F` (struct), `E` (enum), `I` (interface), all with `TD` after the letter.
- **Blueprints:** `BP_` (actor), `WBP_` (widget), `GM_` (game mode), `PC_` (player controller), `GI_` (game instance), `AC_` (actor component), `BI_` (interface).
- **Assets:** `SM_`, `SK_`, `M_`, `MI_`, `T_`, `NS_` (Niagara), `DT_` (DataTable), `L_` (level), `SFX_` (sound), `IA_`, `IMC_`.
- **Project prefix:** `TD` (module `TouchdownMars`).

### 0.3 Tools & workflow
- **Blueprint or C++:** C++ for the mission model, rules, seeded RNG, delay queue, DONKI client and data structs (text the agent can write and the team can diff). Blueprints for scene reactions, crew behaviour, UMG layout and wiring.
- **AI coding assistant** (see `submission.md`, Use of Artificial Intelligence) connected through native Unreal MCP + VibeUE (`http://127.0.0.1:8000/mcp`) and the team's private StraySpark MCP fork (`http://127.0.0.1:13579/mcp`).
- **Version control:** Git + Git LFS, public GitHub repo, Apache 2.0. Commit after every working step. Fab / Megascans / Epic content is git-ignored.
- ⚠️ **Open decision:** C++ needs Visual Studio 2022 with the C++ game workload on every machine that builds. Check this on 13 Nov in the throw-away setup project. If it fails on a machine, that machine works only on Blueprints and content.

---

## 1. Project Architecture

### 1.1 Folder layout
```
TouchdownMars/
  Source/TouchdownMars/
    Model/      TDTypes.h, TDMissionRules.*, TDMissionState.h, TDRandom.*
    Systems/    TDMissionSubsystem.*, TDDonkiClient.*
    Actors/     TDSystemModule.*, TDCrewMember.*, TDGameMode.*, TDPlayerController.*
    Tests/      TDMissionRulesTests.cpp
  Data/Source/  phases.csv, events.csv, ageband.csv, constants.csv, sources.csv, text.csv, donki_archive.csv
  Scripts/      import_data.py, balance_check.py, package.ps1
  Content/TouchdownMars/
    Core/        GM_TD, PC_TD
    Blueprints/  BP_TD_* scene actors and crew
    UI/          WBP_TD_*
    Data/        DT_TD_*
    Maps/        L_TD_Test, L_TD_Outpost, L_TD_MoonOutpost
    Materials/ Meshes/ VFX/ Audio/ NASA/
  docs/         gdd.md, design.md, technical_design.md, level-design.md, cards_check.md, submission.md
```

### 1.2 Core classes
| Class | Parent | Responsibility |
| :-- | :-- | :-- |
| `ATDGameMode` / `GM_TD` | `AGameModeBase` | No default pawn; spawns HUD; tells the subsystem to start a mission with band and seed |
| `ATDPlayerController` / `PC_TD` | `APlayerController` | Mouse cursor visible, input mode Game and UI, adds `IMC_TD_Default`, routes shortcuts |
| `UTDMissionSubsystem` | `UWorldSubsystem` | Holds `FTDMissionState`, deck, delay queue; public functions and delegates for UI and scene |
| `UTDMissionRules` | `UObject` (static functions) | Pure rules; no world, no UI; fully unit-tested |
| `UTDDonkiClient` | `UObject` | HTTP GET to DONKI, JSON parse, archive fallback |
| `ATDSystemModule` | `AActor` | Base scene actor for AIR, GREENHOUSE, SHELTER; `OnLevelChanged` BlueprintImplementableEvent |
| `ATDCrewMember` | `AActor` | Base crew actor; `PlayBehaviour(ETDCrewBehaviour)` BlueprintImplementableEvent |
| `BP_TD_CameraRig` | `AActor` | Fixed CineCamera, opening push-in timeline |

---

## 2. Core Types & Data

### 2.1 Enums
| Enum | Values | Used by |
| :-- | :-- | :-- |
| `ETDSystem` | Air, Greenhouse, Shelter | rules, slots, scene modules |
| `ETDAgeBand` | Cadet, Commander | rules, UI |
| `ETDPhase` | Arrival, FirstWalk, Exploration, Resupply, HardWeek, Return | rules, phase strip |
| `ETDEventFamily` | None, Power, Radiation, Comms, Equipment, Food | deck draw guarantees |
| `ETDCardLabel` | FlownMeasured, NASAPlan, AnalogMission, ModelEstimate | card back |
| `ETDDataStatus` | Live, Archive | DONKI panel, Source Panel |
| `ETDCrewBehaviour` | Working, Worried, Alarm, Shelter, AtLander, Celebrate | crew actors |
| `ETDMissionResult` | InProgress, LostAir, Minimum, Full | report |
| `ETDLanguage` | EN, TR | text lookup |
| `ETDChoiceExtra` | None, StormExposure, ScienceBonus | event choice side effect (added in Phase 1) |

### 2.2 Structs & DataTables
| Struct / Table | Fields | Used by |
| :-- | :-- | :-- |
| `FTDMissionState` | Seed, Band, PhaseIndex, Levels[3], CellsTotal, Allocation[3], PendingGreenhouseDelta, StormExposures, SolsElapsed, PendingMessages, DrawnEventIds, ScienceDone, Result | subsystem, rules, tests |
| `FTDPhaseRow` / `DT_TD_Phases` | PhaseId, SolStart, SolEnd, NameKey, DemandAir, DemandGreenhouse, DemandShelter, EventFamily, OptionalScienceKey, IsBlackout, SourceIds | rules, phase strip |
| `FTDEventRow` / `DT_TD_Events` | EventId, Family, Label, AllowedPhases (bitmask), Bands, QuestionKey, ChoiceAKey, ChoiceADemandDelta[3], ChoiceAExtra, ChoiceBKey, ChoiceBDemandDelta[3], ChoiceBExtra, RevealKey, EventDate, Instrument, MeasurementKey, SourceIds | deck, card widget |
| `FTDAgeBandRow` / `DT_TD_AgeBands` | Band, Cells, Phases, SystemsDecided, AdviceDelayPhases, CardSentences, TermLimit, RitualTaps, ShowDoseDuringPlay | rules, UI |
| `FTDConstantRow` / `DT_TD_Constants` | (row name = Key), Value (double), Unit, SourceId, bIsDesignerValue, SourceDetail | rules, Source Panel |
| `FTDSourceRow` / `DT_TD_Sources` | (row name = SourceId), Title, URL, RetrievedOn, Licence, UsedFor, bIsAIGenerated | Source Panel, README |
| `FTDTextRow` / `DT_TD_Text` | Key, EN, TR | every widget |
| `FTDDonkiRow` / `DT_TD_DonkiArchive` | EventType, ActivityId, TimeUTC, ClassType, SourceURL | archive fallback |
| `FTDSpaceWeather` | Status, FetchedAtUTC, FlareCount, StrongestClass, CMECount, StormCount, Rows | DONKI panel |

All DataTables are imported from `Data/Source/*.csv` by `Scripts/import_data.py`. The CSV is the source of truth.

**As built in Phase 1 (`Source/TouchdownMars/Model/TDTypes.h`):** the first CSV column `Name` is the row name, so no struct field may be called `Name`. Bool columns use the property name with `b` (`bIsBlackout`). Event demand deltas are three ints per choice (`ChoiceADemandAir`, `...Greenhouse`, `...Shelter`) instead of an array; `AllowedPhasesMask` bit N = phase index N; `Bands` is `TArray<ETDAgeBand>` written `(Cadet,Commander)`. `FTDAgeBandRow::TermLimit` 0 = no limit. `FTDMissionState::Levels` and `Allocation` are `TArray<int32>` indexed by `ETDSystem`.

---

## 3. Player System
- **GDD basis:** gdd.md §2 Controls, Camera; §1 Characters (Mission Commander).
- **Architecture:** no pawn. `PC_TD` with cursor; all play goes through UMG buttons and drag operations. `BP_TD_CameraRig` is the view target.
- **Class spec:** `ATDPlayerController`: `SetupInputComponent` binds `IA_TD_NextPhase` (Space, Enter), `IA_TD_Back` (Esc); `bShowMouseCursor = true`; input mode Game and UI, do not hide cursor during capture.
- **Movement values:** none (no movement).
- **Current state:** ✅ Greybox (Phase 3): C++ `ATDGameMode` (no pawn, starts the mission in `StartPlay` after every actor bound), `ATDPlayerController` (cursor, Game and UI input, `IMC_TD_Default`: Space/Enter → `IA_TD_NextPhase`, Esc → `IA_TD_Back`; Esc after the mission ends restarts the same seed until the pause menu exists). Blueprints `GM_TD`, `PC_TD` in `Content/TouchdownMars/Blueprints`; `L_TD_Test` uses `GM_TD` as its World Settings override. Camera rig comes in Phase 5.
- **Required work:** create `PC_TD`, `GM_TD`, `IMC_TD_Default`, camera rig; set as defaults in Project Settings → Maps & Modes.

### 3.0 First-person mode (14.11.2026, team decision)
- `ATDAstronautCharacter` (default pawn while `ATDGameMode::bFirstPerson` is true): capsule 38 x 92 cm, eyes about 1.62 m, FOV 90, no head bob; suited walk 140 cm/s; gravity scale from `constants.csv` `MarsGravityRatio` (0.38, NASA NSSDC), read once the tables load. Input actions and the mapping context are created in code: WASD/arrows move, mouse looks, Space hops, Tab or right mouse button toggles the mission console.
- Mission console: open = Game-and-UI input, cursor on, the UMG decision panel, card and corner buttons shown (`UTDHudWidget::SetConsoleMode`); closed = Game-only input, helmet view. Slate Tab navigation is off so Tab always reaches the game.
- `ATDHelmetHUD` (Canvas): centre dot, the name of what the helmet looks at (actor tags `Inspect_*`, crew members by system; text.csv) and the console hint (urgent when an event card waits).
- Outcome Shots blend from the astronaut's eyes to the shot camera and back; movement and look are locked during a shot. `bFirstPerson = false` restores the fixed `CAM_Main` commander view.
- Level: `Scripts/setup_fps.py` (player start, complex-as-simple collision on NASA and Tripo meshes, Inspect tags). Tripo props and rigged crew: `Scripts/import_tripo_assets.py`, `setup_tripo.py`.

### 3.1 Camera System (14.11.2026, team decision)
- **Rule:** one fixed main gameplay camera for every decision, plus short non-interactive Outcome Shots (2-3 s) only at loop step 4 (the outcome). No free camera, no first or third person control, no cutscenes. The player can never pan, rotate, zoom or move the camera.
- **Main camera:** `CAM_Main` (CineCameraActor, Actor Tag `CAM_Main`) in `L_TD_Outpost`: pitch -38, yaw 0, 41.4 mm on a 23.76 x 13.365 mm filmback (hFOV 32 deg), perspective, no depth of field, no letterbox. The habitat turns to yaw 135 so the camera sits at 45 deg to its long axis while GREENHOUSE, AIR and SHELTER read left to right. Position solved so all three systems and all three crew sit in the top two thirds of a 16:9 frame and clear of the CAPCOM box, phase strip and corner buttons (checked at 1920x1080 and 1366x768). Idle drift: 30 cm along the camera's right axis over 10 s, ping-pong, no rotation; off with Reduce motion and inside shots. The old `TD_CameraRig` push-in is removed from the outpost (the test map keeps it as a fallback).
- **Data:** `FOutcomeShot` (ShotId, EventId, CameraTag, BlendIn 0.4, Hold 2.0, BlendOut 0.4, bAllowedInCadet, ResultTextKey) in `Systems/TDShotDirector.h`; `DT_OutcomeShots` from `Data/Source/outcome_shots.csv` (14 rows; `Scripts/import_data.py`). Result sentences `Shot_*` in text.csv (EN/TR); `<Key>_A` / `<Key>_B` win for choice A / B.
- **Director:** `UTDShotDirector`, an ActorComponent created by `ATDGameMode` (one per world, nothing to place or forget in a level; the GameMode already owns the start of play). `PlayShot(EventId, OutcomeState)` blends with `SetViewTargetWithBlend` (cubic ease in/out) to the actor tagged `CameraTag`, holds, blends back, fires `OnShotFinished` exactly once. `SkipShot()` (any left click during a shot, polled with `WasInputKeyJustPressed`) returns to `CAM_Main` in 0.15 s and fires `OnShotFinished` at once. Missing row or camera: warning with the EventId, no camera move, `OnShotFinished` on the next tick; a safety timer ends any shot one second after its length. No random numbers (the seeded mission stream is untouched). Replays of a ShotId in one playthrough hold 1.2 s; Cadet caps hold at 1.8 s and skips rows with `bAllowedInCadet = false`. No shot in the first 45 s of play. Settings: `bOutcomeShotsEnabled` (default true) and `bReduceMotion` in `UTDSaveGame` (Reduce motion = hard cuts, no drift); no new menu screen.
- **Flow:** the HUD asks for the shot when the card choice is made (our step 4: the decision's outcome), shows the one-sentence result at once, dims the 2D panel (opaque tint, click-through so any click reaches the skip), hides the card face, and flips the card to "What really happened?" on `OnShotFinished`. A successful mission plays `Departure` (S07) before the report. At most one shot per turn.
- **Scene props:** crew stand at TargetPoints tagged `Stage_<CameraTag>_<System>[_A|_B]` during a shot and go back after; `ATDSystemModule` dims when it got no cell this phase in `S03_STORM`; `ATDPlantRack` (placeholder shapes) shows healthy green or stressed yellow and drooping by GREENHOUSE cells vs demand, never growth; `ATDShotProp` shows the message icon over the dish (travels, pauses, no reply) and the cable at the power unit only during their shots.
- **Debug (not in Shipping):** console `td.Shot <ShotId>`, `td.ShotAll`, `td.OutcomeShots 0|1`, `td.ReduceMotion 0|1`; also `-ExecCmds="td.ShotAll"` for demo recording.
- **Level script:** `Scripts/setup_cameras.py` (rerunnable) places `CAM_Main`, the six shot cameras, stage points and props, moves the crew homes, stands the antenna on the ground and parks the rover outside the bottom third.

---

## 4. Mission Model System

### 4.0 Mission Design layer (14.11.2026, challenge switch to "Space Mission Design Game")
- **Model** (`Model/TDMissionDesign.h/.cpp`, pure, no world): `FTDDesignOptionRow` (DT_TD_MissionOptions: slot, role, text keys, constant keys for mass / power out / power draw / cost / capacity / data / science, effects: dust-sensitive, relay, start-level bonus, radiation relief, repair charges, not landed, max count, default, code, based-on), `FTDObjectiveRow` (DT_TD_Objectives: score weights, minimum instruments, minimum end level, science walks, budget key, requirement keys, sandbox), `FTDMissionDesign` (objective, picks with counts, GO per role, realistic model), `FTDDesignBudget` (dashboard numbers, review issues, CML, launchable), `FTDDesignEffects` (what the surface rules read), `FTDMissionScore` (four axes, total, role goals met, why keys).
- **Rules** (`UTDDesignRules`): `Evaluate` (mass = hardware + BVAD consumables x CrewSize x MissionSols + landing system = landed mass x LandingSystemPerLandedKg in the realistic model, orbiter excluded; cells = round((power out - draw) / CellKw), dust cells with DustStormSolarShare on solar output; review list; CML 1-4), `SetPick` (single-pick slots replace, multi-pick for power and instruments, clamp to MaxCount, any change clears GO), `IsRoleGoalMet`, `MakeEffects`, `Score` (efficiency only after lift-off), `MakeDesignCode`.
- **Surface link** (`FTDRulesConfig::Design`): `NewMission` takes cells, shelter / greenhouse start bonus and repair charges from the design; `StartPhase` sets `bDustPhase` from the event row's `bDustStorm` and uses the dust cells; `GetDemand` applies radiation relief and one repair charge; `EndPhase` counts decisions, good decisions and science walks. Solar conjunction blocks every link (relay included), as in reality.
- **Subsystem API**: `BeginDesign(Objective, bRedesign)`, `SetDesignPick`, `SetRealisticModel`, `SetRoleGo`, `LaunchDesign` (needs CML 4; realistic = Commander band, simplified = Cadet band), `GetDesignBudget`, `GetFlownBudget`, `GetScore`, `GetFlights` (session only), `GetWeather(MissionSol)`; delegates `OnDesignChanged`, `OnDesignLaunched`.
- **UI** (`UI/TDDesignRoom.h/.cpp`, Slate): one `STDDesignRoom` widget with four screens (Objectives, Design, Transit, Debrief) on a 1920 x 1080 canvas in an `SScaleBox`; rebuilt after every click. `ATDPlayerController::OpenDesignRoom` / `ShowDebrief` add it to the game viewport with UI-only input; the astronaut and helmet HUD stand down while it is open. `ATDGameMode::bMissionDesign` (default true) opens the design room instead of starting the mission at once.
- **Data sets**: `DT_TD_MarsWeather` (Perseverance MEDA, mission sol N = Perseverance sol 304 + N), shown by `ATDHelmetHUD` as ARCHIVE; RAD Level 2 daily dose and the MQS catalogue values live in `constants.csv`.
- **Tests**: `TD.Design.*` (6). Development builds: `td.AutoPlay` console command and `-TDAutoPlay` (plays the surface mission after landing, for the debrief test).

- **GDD basis:** gdd.md §2 Mechanics 1, 4, 5, 6, 8; §3 Game Flow.
- **Architecture:** `UTDMissionSubsystem` holds state and calls pure `UTDMissionRules`. UI and scene never change state directly; they call subsystem functions and listen to delegates.
- **Class spec:**
  - Functions: `StartMission(ETDAgeBand Band, int32 Seed)`, `PlaceCell(ETDSystem)`, `RemoveCell(ETDSystem)`, `CanEndPhase() const`, `EndPhase()`, `ChooseEventOption(bool bOptionA)`, `CallHome()`, `RestartSameSeed()`, `GetState() const`.
  - Delegates (dynamic multicast): `OnAllocationChanged(FTDMissionState)`, `OnLevelsChanged(FTDMissionState, TArray<ETDSystem> Changed)`, `OnPhaseStarted(ETDPhase)`, `OnEventDrawn(FName EventId)`, `OnEventResolved(FName EventId, FText CauseSentence)`, `OnMessageArrived(FText)`, `OnMissionEnded(ETDMissionResult)`.
  - Rules: `ApplyAllocation(State, PhaseRow, EventDelta) → State`, `DrawEvent(State, Deck, Rng) → EventId`, `ResolveCause(StateBefore, StateAfter) → CauseKey`.
- **Numbers & formulas:** LOCKED values (13.11.2026) from gdd.md §2 Mechanic 1: delta = cells - demand; delta ≥ +1 → level +1; 0 → 0; -1 → -1; ≤ -2 → -2; clamp 0..4. Greenhouse delta applied at next phase start. Background dose = SolsElapsed × 0.65 mSv (constant row `MarsSurfaceDose`). Seeded RNG: `FRandomStream(Seed)`; seed 4 digits (1000-9999).
- **Current state:** ✅ Built in Phase 2 (13.11.2026): `Model/TDMissionRules.*`, `Systems/TDMissionSubsystem.*`, `Tests/TDMissionRulesTests.cpp` (11 tests green), `Scripts/balance_check.py`.
- **Designer decisions made while building (team to confirm):**
  - Cadet merges Arrival and FirstWalk by skipping FirstWalk; the merged Arrival ends at sol 5. So Cadet has no Power-family phase; the deck guarantee for Cadet is Radiation + Comms.
  - Cadet locked system per phase comes from `phases.csv` column `CadetLockedSystem` (Shelter, Shelter, Greenhouse, Shelter, Greenhouse, Shelter). A locked system needs no cells and keeps its level (no free healing).
  - Events are drawn by the phase's `EventFamily`; if no card of that family is left, any allowed card is drawn. Same seed + phase → same card (`FRandomStream(Seed*97 + Phase*7919 + 13)`).
  - Radiation exposure: +1 when the chosen option has `StormExposure`/`ScienceWithExposure`, or a Radiation card ends with SHELTER below `ShelterSafeLevel` (counted once per phase).
  - Report stamps: Crew = lift-off and 0 exposures; Base = lift-off and every system ≥ 2; Emergency = no system below 2 at the end of any event phase; Bonus = optional science done or harvest. Lift-off with all four = Full, otherwise Minimum.
  - Mission Control: the reply's arrive phase is the playable phase `AdviceDelayPhases` steps later; nothing is delivered or sent in a blackout phase; held replies arrive in the next non-blackout phase.
- **Required work:** C++ types and rules; automation tests `TD.Model.*` (no-free-choice test: for every event row, both choices must lower at least one system or add an exposure); `Scripts/balance_check.py` that plays 1,000 seeds with random and greedy allocations and prints the loss rate per phase (target: phase 1 loss 0 %, overall random-play loss between 20 % and 50 %, `[DESIGNER VALUE]`).

---

## 5. Event Card System
- **GDD basis:** gdd.md §2 Mechanic 2; §1 Main Mechanics 2.
- **Architecture:** deck = rows of `DT_TD_Events` filtered by band and phase; draw guarantees one Power, one Radiation and one Comms event per game. `WBP_TD_EventCard` shows the front, waits for a choice, plays the flip, shows the back, waits for close.
- **Class spec:** `WBP_TD_EventCard`: `Setup(FName EventId)`, `OnChoice(bool bA)` → subsystem; `PlayFlip()`; back side builds lines from row fields and `DT_TD_Sources`.
- **Numbers & formulas:** demand delta +1 normal, +2 severe (LOCKED 13.11.2026). At least 8 cards in the deck, target 12.
- **Current state:** 🔲 Not started.
- **Required work:** struct, CSV columns, widget, flip animation, priority rule line "Crew first, then base, then science." on every front.

---

## 6. Mission Control Delay System
- **GDD basis:** gdd.md §2 Mechanic 3.
- **Architecture:** `PendingMessages` array of `{SentPhase, ArrivePhase, TextKey}` in state. `CallHome()` adds one; `EndPhase()` delivers due messages through `OnMessageArrived`. Blackout phase: no delivery.
- **Class spec:** `WBP_TD_Capcom` (portrait + line), `WBP_TD_MessageTrack` (dot animation by `ArrivePhase - SentPhase`).
- **Numbers & formulas:** Commander Mars delay 1 phase; Cadet 0; Moon 0. Light-time constants for display: 11.0 min (14 Nov 2026), 3.2-22.3 min range.
- **Current state:** 🔲 Not started.
- **Required work:** state fields, delivery in `EndPhase`, widgets, "Message still on its way" in the report.

---

## 7. AI
- **States:** crew behaviours Working, Worried, Alarm, Shelter, AtLander, Celebrate.
- **Perception:** none.
- **Classes:** `ATDCrewMember` + three Blueprints; each listens to `OnLevelsChanged` for its system and to `OnEventResolved` for radiation (Shelter) and `OnMissionEnded` (AtLander / Celebrate). Movement only along fixed splines placed in the level.
- **Current state (14.11.2026):** ✅ Surface crew AI and base operations (gdd.md Main Mechanics 14).
  - `Model/TDOperations.h/.cpp` (`UTDOps`, pure rules): `FTDTaskRow` (tasks.csv → DT_TD_Tasks), `FTDSiteRow` (sites.csv → DT_TD_Sites), `FTDOpsState` (crew plans, done tasks, water, regolith, dust on the arrays, weather mast), `CanPlan` (phase mask, design option, prerequisite, hours, spacewalk hours with the rest-sol rule, dust storm, buddy), `Plan`/`Unplan`/`AutoPlan` (role priorities), `Finish`, `ApplyEffect` (levels, `FTDMissionState::OpsRadiationRelief`, science walks, harvest).
  - `UTDMissionSubsystem`: `PlanTask`, `AutoPlanTasks`, `GetTaskBlock`, `RunPhase` (builds each AI crew member's steps: walk path, work seconds, inside or not; the busiest plan runs about 14 s), `FinishRun` (effects, report, `EndPhase`), `CompleteOwnTask` (player at the site, E), solar dust after a storm phase, `OnOpsChanged`/`OnOpsReport`.
  - `Systems/TDWalkGrid.h/.cpp`: walkable grid (100 cm cells over the base) built once from the landscape height and chest-height overlap tests; A* with no corner cutting, then straight-line shortening. No nav mesh needed.
  - `ATDCrewMember`: `WalkPath`, `RunSteps` (walk → work timer with progress → next step), `SetInside`, `Say` (barks), `StatusText`/`StatusProgress` for the HUD; level reactions do not teleport a crew member who is on a task.
  - `UI/TDBaseComputer.h/.cpp` (Slate, NASA-punk palette): crew cards, task board, setup checklist, base status, run button. `ATDPlayerController::ToggleBaseComputer`/`RunPhase`; B and E bound in `ATDAstronautCharacter`.
  - `ATDHelmetHUD::DrawOps`: crew labels over the heads (name, task, progress, bark), your task markers, the E prompt, the time-lapse banner.
  - Deployed parts: `UTDMissionSubsystem::RefreshOpsScene` shows actors tagged `OpsShow_<TaskId>` once the task is done (`OpsHide_` the other way), combined with `DesignShow_`; the walk grid is rebuilt after a change.
  - EVA buddy: `UTDMissionSubsystem::TickEscort` (1 s timer) picks the nearest idle crew member when the player is `EscortStartDistanceCm` from the hub; `ATDCrewMember::StartEscort/StopEscort/StepEscort` keep them beside the player (control rotation forward, +160 cm ahead, +230 cm right), hurry by distance, never teleport; callouts `Callout_<Site>` once per site, `Escort_Dust`, `Escort_Far`.
  - Rover traverse: `ETDTaskEffect::Expedition` (+`ExpeditionScienceWalks`, `FTDOpsState::RoverChargingPhase`), `ETDTaskBlock::RoverCharging`; the rover actor hides during the run.
  - Fatigue: `UTDOps::Plan(..., bOverrideRest)`, `RecomputeRestBroken`, `CrewPhaseHours` (`FatigueWorkShare`), `FTDOpsState::Fatigue/RestBroken/RestBreaks`; debrief line `Broke_RestRule`.
  - Tests: `TD.Ops.RoverRechargesAfterTraverse`, `TD.Ops.BrokenRestRuleTires` (29 tests in all).
- **Gamepad (15.11.2026):** pad keys mapped in code next to the keyboard (`ATDAstronautCharacter::CreateInput`: Gamepad_Left2D/Right2D with dead zone, scalar and delta-time modifiers; A, X, Y, View), Menu runs the phase (`IMC_TD_Pad` added by `ATDPlayerController`). `UI/TDGamepadCursor` (subclass of `FAnalogCursor`, registered as a Slate input preprocessor) drives the cursor only while a mouse screen is open (`ATDPlayerController::WantsPadCursor`). Dev flag `-TDPadTest` injects real pad events into Slate and logs cursor move, A click, stick walk, View/B. The module builds without unity (`bUseUnity = false`) so each file's anonymous-namespace helpers stay separate.

---

## 8. UI System
- **Widgets:** `WBP_TD_HUD` (root), `WBP_TD_PhaseStrip`, `WBP_TD_Capcom`, `WBP_TD_MessageTrack`, `WBP_TD_PowerPanel`, `WBP_TD_PowerCell`, `WBP_TD_SystemSlot`, `WBP_TD_NextPhaseButton`, `WBP_TD_EventCard`, `WBP_TD_SourcePanel`, `WBP_TD_SunToday`, `WBP_TD_MissionReport`, `WBP_TD_BackToLander`, `WBP_TD_Settings`, `WBP_TD_GhostHand`, `WBP_TD_CornerButtons`. All follow `design.md` tokens.
- **How UI gets data:** subsystem delegates and `DT_TD_Text` lookup by key and current language; never Tick polling.
- **Current state:** 🔄 Greybox (Phase 3): C++ bases in `UI/TDWidgets.h` (logic, colors and sizes from `UI/TDDesignTokens.h`), layouts `WBP_TD_HUD`, `WBP_TD_PhaseStrip`, `WBP_TD_PowerPanel`, `WBP_TD_PowerCell`, `WBP_TD_SystemSlot`, `WBP_TD_NextPhaseButton` built by `Scripts/build_greybox_ui.py`. Only `WBP_TD_HUD` listens to the subsystem delegates and refreshes its children. Greybox input: click a slot = place one cell, small button under the slot = take one back (the click-a-cell-then-a-slot path and drag come with the opening minute in Phase 5). Cells needed are shown as outlined squares, placed cells as filled squares; no numbers on the decision layer (numbers only in the debug block).
- **Required work:** build widgets with VibeUE `umg-widgets`; drag-and-drop with `UDragDropOperation` for cells, plus click-cell-then-click-slot as the main path.

---

## 9. Save & Progress
- **What is saved:** language, age band, sound on/off, reduce motion, last seed, last mission's four report lines (for "last time vs this time").
- **When:** on Settings change and on mission end.
- **Class:** `UTDSaveGame : USaveGame`, slot `TD_Settings`. No personal data, no names, no analytics.

---

## 10. Audio
- **Sound classes / mixes:** `SC_TD_UI`, `SC_TD_World`, `SC_TD_Voice` (if voice ships), `SC_TD_Music`; one master mix; Mute sets the master to 0.
- **Who plays which sound:** widgets play `SND_UI_*` tokens and card sounds; scene actors play system loops and alarms; CAPCOM widget plays `SFX_TD_RadioIn`.

---

## 11. DONKI Client
- **GDD basis:** gdd.md §2 Mechanic 7; challenge requirement "do not present static data as live".
- **Architecture:** `UTDDonkiClient::Fetch()` sends three GETs (`FLR`, `CME`, `GST`) to `https://ccmc.gsfc.nasa.gov/DONKI/WS/get/{type}?startDate=YYYY-MM-DD&endDate=YYYY-MM-DD` for the last 30 days; parses JSON; fires `OnSpaceWeatherReady(FTDSpaceWeather)`.
- **Numbers:** timeout 5 s each; date range ≤ 60 days; no API key.
- **Fallback:** any failure → `Status = Archive`, rows from `DT_TD_DonkiArchive` (September 2017 events, e.g. `2017-09-10T15:35:00-FLR-001`, X8.2).
- **Current state:** 🔲 Not started.
- **Required work:** add `HTTP` and `Json` to `Build.cs`; client; panel; disclaimer "DONKI is a research prototype catalogue; it does not give dose values."

---

## 12. Performance Rules
- Avoid Tick for gameplay and UI logic. Use events, timers and delegates.
- Packaged build must hold 30 fps at 1920x1080 on the weakest team laptop; check at every package gate with `stat unit` and the VibeUE `profiling` skill.
- If below 30 fps: first turn off Lumen and bake lighting, then reduce Megascans texture size to 2K, then cull foliage.
- Landscape: one component size suited to a single fixed view; no World Partition.
- Package size target: under 4 GB (MISSING: upload limit of the chosen host, checked at Package 0).

---

## 13. Testing
- **Test map:** `L_TD_Test` (every new mechanic is tested here first).
- **How each mechanic is tested:**
  - Allocation and levels: automation tests `TD.Model.Allocation.*` + PIE in `L_TD_Test` with the debug text block.
  - No-free-choice: automation test over all event rows.
  - Deck guarantees: test that 1,000 seeds always draw one Power, one Radiation, one Comms event.
  - Delay: test message arrival by band and blackout.
  - DONKI: PIE with network on (LIVE) and with a bad URL (ARCHIVE).
  - Opening minute: a team member who did not build it plays it cold; first purposeful click under 10 s.
  - Child tests: protocol in the team report (Smileyometer before/after, time to first click, replay, three "show me" questions).

---

## 14. Open Decisions
| Question | Options | Decision |
| :-- | :-- | :-- |
| Cell rule numbers | PROPOSAL in gdd.md §2 / other | LOCKED 13.11.2026: proposal kept |
| Age-band choice at launch | default Commander + corner switch / first-screen choice | LOCKED 13.11.2026: default Commander + corner switch and Settings |
| C++ toolchain on every laptop | yes / Blueprint-only on some machines | Main laptop OK (VS 2022 17.14, 13.11); other laptops not tested |
| Lighting | Lumen / baked | decided at Package 1 by fps |
| Download host | GitHub Releases (2 GB per file) / itch.io / other | MISSING: checked at Package 0 |
| Crew rig and animations | Tripo auto-rig + retargeted UE animations / static poses | MISSING: test on Sat morning |
| Voice | none / English TTS declared as SI / adult team member | cut first if late |
