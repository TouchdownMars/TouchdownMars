# Touchdown Mars: First Light · Production Pipeline (State Machine)

> **Canonical design source:** `gdd.md`. Companions: `technical_design.md` (how the code works) and `level-design.md` (where things are). Tool IDs in the Tools lines: U Epic Unreal MCP, V VibeUE, VS VibeUE skill, S StraySpark MCP fork, SH shell, F files, M manual. This file is the **order of operations**: small phases, in order, each with a hard check before the next one starts.
>
> **How to use this file:** treat each Phase as a node in a state machine. A phase may not start until its `Depends on` phases are `DONE`. Every phase ends with a **Verification Gate**: the feature is tested with Play In Editor (or the package) and a team member confirms before moving on. Never build UI before the model it shows. Never build a level before its mechanics work in the test map.
> **Clock:** times are Istanbul time (TRT). Hackathon window 14.11.2026 09:00 → 15.11.2026 23:59. Jury time is an assumption (Sunday afternoon); MISSING: real jury time from the Local Lead.

**Legend:** `[BP]` Blueprint work · `[C++]` code · `[ART]` assets · `[UI]` interface · `[AUDIO]` sound · `[DATA]` CSV and sources · `[DOC]` submission material · `⚠️` risk / decision
**Roles:** A = model and logic · B = scene, UI, packaging · C = content, sources, child tests

---

## MACRO-STAGE MAP
```
STAGE 1: PROTOTYPE        STAGE 2: FULL GAME          STAGE 3: POLISH & SUBMIT
(model + loop work)       (all content, dev quality)  (art, tests, final build)
Phases 0 to 4             Phases 5 to 10              Phases 11 to 17
      |                          |                          |
 [PROTOTYPE GATE] ------> [CONTENT GATE] ------------> [FINAL BUILD + SUBMIT]
   Sat 14:30                Sun ~01:00                  Sun 15:00
```

**Stage 1 goal:** a six-phase allocation loop with seeded events plays in `L_TD_Test` and the model tests pass.
**Stage 2 goal:** the full game in greybox: opening minute, at least eight sourced cards, delayed advice, Mission Report, lose and restart, both age bands, DONKI panel with archive, Source Panel, English and Turkish text; tested once with children.
**Stage 3 goal:** realistic art and audio, second child test, top three fixes, final Windows package, 30 s demo, full gameplay video, landing page, project page submitted.

---

## PROGRESS TRACKER
> Update this table after every work session. Status: `⬜ NOT STARTED` · `🔄 IN PROGRESS` · `✅ DONE` · `⏸️ BLOCKED`.
> When you resume, start from the first row that is not `✅`.

| # | Phase | Stage | Status | Last update | Notes |
| :-- | :-- | :-- | :-- | :-- | :-- |
| 0 | Lock, project setup, repo, Package 0 & test map | 1 | 🔄 IN PROGRESS | 13.11 17:55 | started early on the team's decision. Done: C++ project, plugins, MCP x2, `L_TD_Test`, Package 0 (597 MB, runs). Open: team locks, first commit, GitHub repo, upload, project page draft, second-laptop test |
| 1 | Core types, CSV schemas, data import | 1 | 🔄 IN PROGRESS | 13.11 19:10 | gate passed technically: `import_data.py` twice, 7 tables, counts match, constants rule OK. Open: Mars Trek download (Role C), team confirmation |
| 2 | Mission model + automation tests | 1 | 🔄 IN PROGRESS | 13.11 19:55 | 11/11 `TD.Model.*` green; balance (no cards yet): random loss Commander 14.3 %, Cadet 10.8 %, phase 1 loss 0 %. Open: team confirmation of the designer decisions in technical_design.md §4 |
| 3 | Greybox loop UI in `L_TD_Test` | 1 | 🔄 IN PROGRESS | 13.11 21:00 | built and driven in PIE: six phases to Full (sol 31) and LostAir in phase 2; waiting for the team's click test (gate question) |
| 4 | 🏁 Prototype gate | 1 | ⏸️ BLOCKED | 13.11 | team chose to test at the end ("non-stop"); agent continued without the sign-off |
| 5 | Outpost greybox, camera, opening minute | 2 | 🔄 IN PROGRESS | 13.11 23:30 | `L_TD_Outpost` greybox, camera rig push-in, opening minute, ghost-hand hint, dark start; gate (10 s first click) needs a human |
| 6 | Event deck, card widget, flip, 8 sourced cards | 2 | 🔄 IN PROGRESS | 13.11 23:30 | 14 sourced cards (checked 13.11, `docs/cards_check.md`), card widget + flip; Role C should re-read the cards |
| 7 | Mission Control delay, CAPCOM, Call home | 2 | 🔄 IN PROGRESS | 13.11 23:30 | CAPCOM, message track, Call home, blackout "No signal"; tested in PIE through the subsystem |
| 8 | 📦 Package 1 + child test 1 | 2 | 🔄 IN PROGRESS | 13.11 23:45 | Package 1 (602 MB) runs, mission starts, DONKI LIVE; child test 1 is a human task |
| 9 | Report, lose/restart, Cadet band, DONKI, Source Panel, TR/EN | 2 | 🔄 IN PROGRESS | 13.11 23:30 | report, back to lander, DONKI client + panel, Source Panel, settings/pause, save game, TR/EN, sounds; all seen in PIE |
| 10 | 🏁 Content gate (Package 2) | 2 | 🔄 IN PROGRESS | 14.11 00:15 | Package 2: 602 MB, runs, DONKI LIVE; SHA-256 must be taken of the final zip (the root exe is only a launcher); upload + team sign-off open |
| 11 | Art pass: Mars Trek terrain, Megascans, NASA + Tripo models, light | 3 | 🔄 IN PROGRESS | 14.11 01:30 | done: Mars Trek HiRISE Jezero DEM → 1 km landscape, procedural regolith `M_TD_MarsGround`, NASA 3D habitat + Z2 suits (insignia removed), dome, regolith shelter mound. + 90 free Megascans rocks (4 assets, Fab Personal licence, `/Game/Fab/Free`, git-ignored). Open: Tripo props, crew animation, fps on weakest laptop |
| 12 | Audio pass | 3 | 🔄 IN PROGRESS | 13.11 23:30 | 11 synthesized SFX (team-made script) on place/take/choose/flip/radio/alarm/lift-off/stamp; no system loops, no music (cut list) |
| 13 | 📦 Package 3 + child test 2 | 3 | 🔄 IN PROGRESS | 14.11 02:10 | Package 3: 641 MB, runs with terrain, NASA models and Megascans rocks; child test 2 is a human task |
| 14 | Top three fixes from tests | 3 | ⬜ NOT STARTED | | |
| 15 | 🧊 Feature freeze | 3 | ⬜ NOT STARTED | | Sun 12:30 |
| 16 | Final package, videos, landing page, project page | 3 | 🔄 IN PROGRESS | 14.11 00:40 | drafts: `docs/submission.md`, `model_card.md`, `teacher_card.md`, `site/index.html`, README; videos, links, hash = TODO |
| 17 | 🏁 Submit for judging (then update until 23:59) | 3 | ⬜ NOT STARTED | | |
| 18 | Optional: Moon mode | 3 | ⬜ NOT STARTED | | cut first (cut list); not started |

---

# STAGE 1: PROTOTYPE

## PHASE 0: Lock, Project Setup, Repo, Package 0 & Test Map
**Objective:** lock the design numbers and prove the full build-and-share path works in the first 90 minutes.
**Depends on:** none. Starts 14.11.2026 09:00, never earlier.
**Time:** Sat 09:00-10:30, all roles together.
**Tools**: M (team locks, Project Browser) · F (`.uproject`, ini, `.mcp.json`, docs) · SH (plugin copy, `Build.bat`, git, `package.ps1`) · S `new_level`/`create_actor`/`list_actors` · V `ProjectSettingsService`.

**Tasks:**
- [DOC] Team locks: the three systems, the cell rule numbers (gdd.md §2 PROPOSAL or changed), the six phases, the age-band launch choice, the crew names. Write the decisions into `gdd.md` and remove the matching `MISSING:` and `PROPOSAL:` markers.
- [C++] Create the Unreal 5.8 C++ project `TouchdownMars`; enable ModelContextProtocol, ToolsetRegistry, PythonScriptPlugin, EditorScriptingUtilities, MovieRenderPipeline; `Build.cs` modules HTTP, Json, JsonUtilities, EnhancedInput; copy `Plugins/VibeUE` and the private `Plugins/UnrealMCPServer` fork (both git-ignored); MCP ini block for the editor tooling (local only).
- [DOC] Create the public GitHub repo; first commit: `LICENSE` (Apache 2.0), `.gitignore` (Binaries, Intermediate, Saved, DerivedDataCache, Builds, Content/Fab, Content/Megascans, Plugins/UnrealMCPServer, Plugins/VibeUE), `.gitattributes` for LFS, `README.md` stub, these docs.
- [BP] Create `L_TD_Test` with a camera and a text actor.
- [BP] Package an empty Windows build (Package 0); upload to the chosen host; open the link in a private window without login.
- [DOC] Open the team page, Project tab; save a draft (name, one-line summary) with "Save Progress".
- ⚠️ Confirm MCP: list actors in `L_TD_Test` from the AI coding assistant.

**Deliverable:** locked numbers in `gdd.md`; repo with first commit; Package 0 download link that works logged out; project page draft saved.
**Verification Gate:** a team member downloads Package 0 on another laptop and it opens. ✋ Confirm with the team before Phase 1.

---

## PHASE 1: Core Types, CSV Schemas, Data Import
**Objective:** every type and table the model needs exists and imports from CSV.
**Depends on:** Phase 0.
**Time:** Sat 10:30-11:30. Role A (types, import), Role C (CSV content start).
**Tools**: F (`TDTypes.h`, CSV) · SH full build · U `execute_python_code` → `Scripts/import_data.py` · S `get_datatable_rows` · SH `curl` DONKI · M Mars Trek download.

**Tasks:**
- [C++] `TDTypes.h`: all enums and structs from technical_design.md §2.
- [DATA] `Data/Source/phases.csv`, `ageband.csv`, `constants.csv`, `sources.csv`, `text.csv` with headers and the locked rows; `events.csv` with headers.
- [DATA] `Scripts/import_data.py` creates or reimports every `DT_TD_*` from its CSV.
- [DATA] Role C reruns the DONKI and Horizons queries and records retrieval dates in `sources.csv`; downloads the Mars Trek elevation for the outpost site and records the product name and URL.

**Deliverable:** all `DT_TD_*` assets exist with rows.
**Verification Gate:** `import_data.py` runs twice without errors and row counts match the CSVs. ✋ Confirm with the team.

---

## PHASE 2: Mission Model + Automation Tests
**Objective:** the full six-phase model runs headless with seeded randomness.
**Depends on:** Phase 1.
**Time:** Sat 11:30-14:30. Role A.
**Tools**: F (C++) · SH build + `Automation RunTests TD.Model` (UnrealEditor-Cmd, nullrhi) · U `execute_python_code` for `balance_check.py`.

**Tasks:**
- [C++] `UTDMissionRules` (allocation, greenhouse delay, dose count, lose check, result).
- [C++] `UTDMissionSubsystem` with functions and delegates from technical_design.md §4.
- [C++] Tests `TD.Model.*`: allocation table, clamp, phase 1 cannot be lost, same seed gives same game, deck guarantees, no-free-choice over all event rows.
- [DATA] `Scripts/balance_check.py` playing 1,000 seeds; print loss rate by phase.

**Deliverable:** model + passing tests + balance report.
**Verification Gate:** all `TD.Model.*` tests green; balance report pasted into the team log. ✋ Confirm with the team.

---

## PHASE 3: Greybox Loop UI in L_TD_Test
**Objective:** a person can play all six phases with clicks in the test map.
**Depends on:** Phase 1 (UI skeleton can start in parallel with Phase 2; wiring needs Phase 2).
**Time:** Sat 10:30-14:30. Role B.
**Tools**: VS `umg-widgets`/`enhanced-input`/`blueprints`/`pie-testing` · V `WidgetService`, `InputService`, `BlueprintService` · S `create_blueprint` (C++ parents), `create_static_mesh_actor` · V `start_pie` · U `capture_image`.

**Tasks:**
- [UI] `WBP_TD_HUD`, `WBP_TD_PowerPanel`, `WBP_TD_PowerCell`, `WBP_TD_SystemSlot`, `WBP_TD_NextPhaseButton`, `WBP_TD_PhaseStrip` with `design.md` tokens.
- [BP] `GM_TD`, `PC_TD`, `IMC_TD_Default`, `IA_TD_NextPhase`, `IA_TD_Back`.
- [BP] Three cube system modules reacting to `OnLevelsChanged` (light intensity by level).
- [BP] Debug text block printing the state after every phase.

**Deliverable:** clickable loop in `L_TD_Test`.
**Verification Gate:** "L_TD_Test'te altı evre boyunca güç paylaştırınca durum doğru değişiyor mu?" ✋ Confirm with the team.

---

## PHASE 4: 🏁 Prototype Gate
**Objective:** confirm the core loop is worth building on.
**Depends on:** Phases 2 and 3.
**Time:** Sat 14:30.
**Tools**: M (play) · F (the team log).

**Tasks:**
- Each role plays two full missions; note one problem each.
- Check pillar P1: every phase forces a trade-off after phase 1.

**Deliverable:** signed-off prototype; problems listed in the team log.
**Verification Gate:** all three roles say yes. ✋ Confirm with the team.

---

# STAGE 2: FULL GAME

## PHASE 5: Outpost Greybox, Camera, Opening Minute
**Objective:** the first 60 seconds play exactly as specified in gdd.md §1 Walkthrough.
**Depends on:** Phase 4.
**Time:** Sat 14:30-18:30. Role B.
**Tools**: S `new_level`, `create_basic_level`, `create_static_mesh_actor`, `batch_transform`, `place_actor_on_ground`, Sequencer tools · V `WidgetService` animations · F (idle timer in C++).

**Tasks:**
- [BP] `L_TD_Outpost` greybox with block meshes at final positions (level-design.md §3 Layout); `BP_TD_CameraRig` with push-in.
- [BP] `BP_TD_PowerUnit` energy lines; crew as capsules with behaviour placeholders.
- [UI] Opening sequence: dark start, "You are Stay", pulsing cell, `WBP_TD_GhostHand`, systems named one by one, second cell, Call home appears, first card question.
- [UI] Idle rule: ghost hand after 6-8 s without a purposeful click.

**Deliverable:** opening minute in `L_TD_Outpost`.
**Verification Gate:** "Oyunu açan biri 10 saniye içinde ilk hücreyi HAVA'ya koyabiliyor mu?" (tested by a member who did not build it). ✋ Confirm with the team.

---

## PHASE 6: Event Deck, Card Widget, Flip, Eight Sourced Cards
**Objective:** events with real sourced reveals drive the trade-offs.
**Depends on:** Phase 4.
**Time:** Sat 14:30-18:30. Role A (deck logic), Role B (widget), Role C (cards).
**Tools**: F + SH build/tests (deck) · V `WidgetService` (card + flip) · M Role C sources · F CSV rows · U `import_data.py` · SH `curl -I` source URLs.

**Tasks:**
- [C++] Deck draw with guarantees; choice effects.
- [UI] `WBP_TD_EventCard` front, flip, back; priority rule line.
- [DATA] Role C writes at least eight cards in `events.csv` and `text.csv` (EN and TR), each after opening and reading its source page; every number cross-checked with `constants.csv`; banned numbers never used.

**Deliverable:** eight playable cards.
**Verification Gate:** every card's back shows date, instrument, measurement, label and a working source URL. ✋ Confirm with the team.

---

## PHASE 7: Mission Control Delay, CAPCOM, Call Home
**Objective:** advice arrives late on Mars and never in the blackout.
**Depends on:** Phase 4.
**Time:** Sat 14:30-18:30 (Role A logic), Sat 20:30-01:00 (Role B widgets).
**Tools**: F + SH build/tests (delay queue) · V `WidgetService` (CAPCOM, message track).

**Tasks:**
- [C++] Pending message queue in state; delivery in `EndPhase`.
- [UI] `WBP_TD_Capcom`, `WBP_TD_MessageTrack` with the travelling dot and stamped send phase.

**Deliverable:** Call home works in both bands.
**Verification Gate:** Commander on Mars: reply one phase later; phase 5: "No signal". ✋ Confirm with the team.

---

## PHASE 8: 📦 Package 1 + Child Test 1
**Objective:** a real build in front of real children.
**Depends on:** Phases 5, 6, 7.
**Time:** Sat 18:30-20:30. All roles.
**Tools**: S `validate_assets`, `get_map_check_errors` · SH `package.ps1` · M child test · F notes.

**Tasks:**
- [BP] Package 1; open on the weakest laptop; note fps.
- [DOC] Child test 1: two children aged 7-9 (friend pair), written guardian consent, an adult present, protocol from the team report; no photo, video or voice recording.
- [DOC] Record time to first purposeful click, replay, Smileyometer before/after, three "show me" answers, top three problems.

**Deliverable:** Package 1, anonymous test notes.
**Verification Gate:** "Paket en zayıf laptopta açıldı mı ve 30 fps'nin üstünde mi?" and "Çocuk testinden çıkan en önemli üç sorun yazıldı mı?" ✋ Confirm with the team.

---

## PHASE 9: Report, Lose/Restart, Cadet Band, DONKI, Source Panel, TR/EN
**Objective:** every remaining must-have feature in greybox quality.
**Depends on:** Phase 8.
**Time:** Sat 20:30-01:00. Role A (Cadet parameters, test fixes, balance), Role B (report, DONKI panel, Source Panel), Role C (source data, model card, remaining cards, teacher card start).
**Tools**: V `WidgetService` (report, DONKI, Source Panel, settings) · F + SH (Cadet, `UTDDonkiClient`, `UTDSaveGame`) · U `import_data.py` (text) · SH `curl` DONKI.

**Tasks:**
- [UI] `WBP_TD_MissionReport` (four stamps, patch, seed, last-time comparison), `WBP_TD_BackToLander`, restart same seed.
- [C++] Cadet band parameters; first three child-test fixes.
- [C++] `UTDDonkiClient`; `[UI]` `WBP_TD_SunToday` with LIVE/ARCHIVE labels and disclaimer.
- [UI] `WBP_TD_SourcePanel` from `DT_TD_Sources` and `DT_TD_Constants`.
- [UI] Language switch EN/TR; `WBP_TD_Settings`; `UTDSaveGame`.
- [DATA] Cards up to 12 if verified; model card (which numbers are NASA, which are designer values).

**Deliverable:** feature-complete greybox game.
**Verification Gate:** a full mission in each band and each language without a blocker; DONKI tested online and offline. ✋ Confirm with the team.

---

## PHASE 10: 🏁 Content Gate (Package 2)
**Objective:** a complete, shareable fallback build exists before sleep.
**Depends on:** Phase 9.
**Time:** Sun ~01:00, then rotating sleep until 07:00.
**Tools**: S validation · SH `package.ps1`, git · M upload.

**Tasks:**
- [BP] Package 2; upload; keep the link (this is the fallback submission if art fails).
- [DOC] Update tracker and the team log; commit.

**Deliverable:** Package 2 online.
**Verification Gate:** link opens logged out on a second laptop. ✋ Confirm with the team.

---

# STAGE 3: POLISH & SUBMIT

## PHASE 11: Art Pass
**Objective:** realistic Mars world with a clear schematic decision layer.
**Depends on:** Phase 10.
**Time:** Sun 07:00-09:30. Role B (scene), Role A (balance), Role C (teacher card, Turkish terms).
**Tools**: SH Python (DEM → heightmap) · V `LandscapeService`, `LandscapeMaterialService`, `FoliageService`, Niagara · M Megascans/NASA 3D/Tripo · S `import_asset_with_settings`, light/sky/fog/post tools, `get_render_stats`.

**Tasks:**
- [ART] Mars Trek elevation → landscape (VibeUE `landscape`); Megascans surface materials and rocks (VibeUE `fab`, into `Content/Fab/`).
- [ART] NASA 3D Resources rover and hardware; Tripo props and crew suit (no NASA marks); record every asset in `sources.csv` (Tripo marked AI-generated).
- [ART] System lights, sky, dust and storm VFX.
- ⚠️ Crew animation: if Tripo rig + retarget fails by 08:30, use static poses that switch by behaviour.
- [BP] `stat unit` check; if under 30 fps, Lumen off and baked light.

**Deliverable:** art in `L_TD_Outpost`.
**Verification Gate:** screenshot review against `design.md`; fps check on the weakest laptop. ✋ Confirm with the team.

---

## PHASE 12: Audio Pass
**Objective:** every interaction has motion and sound.
**Depends on:** Phase 10.
**Time:** Sun 07:00-09:30 (Role B, after art tasks).
**Tools**: S `import_asset_with_settings` · V `SoundCueService`/`MetaSoundService` · F `sources.csv`.

**Tasks:**
- [AUDIO] UI sound tokens, system loops, card, radio, alarm, door, wind, lift-off, stamp.
- [AUDIO] Record every sound source and licence in `sources.csv`; AI-generated audio declared.

**Deliverable:** sound on every interaction.
**Verification Gate:** play one mission with eyes closed for 30 s; every click is heard. ✋ Confirm with the team.

---

## PHASE 13: 📦 Package 3 + Child Test 2
**Objective:** test the near-final game with older children.
**Depends on:** Phases 11 and 12.
**Time:** Sun 09:30-11:00.
**Tools**: S validation · SH `package.ps1` · M child test · F notes.

**Tasks:**
- [BP] Package 3; weakest-laptop check.
- [DOC] Child test 2: three children aged 10-12, one at a time, same protocol; before/after notes.

**Deliverable:** Package 3, test table.
**Verification Gate:** "Çocuk testinden çıkan en önemli üç sorun yazıldı mı?" ✋ Confirm with the team.

---

## PHASE 14: Top Three Fixes
**Objective:** fix what the children struggled with most.
**Depends on:** Phase 13.
**Time:** Sun 11:00-12:30. Role A and B fix; Role C writes the test table and before/after notes.
**Tools**: F + SH (C++ fixes, tests) · V `WidgetService`/`BlueprintService` (UI/BP fixes) · V PIE.

**Tasks:**
- [C++]/[UI] Fix the three highest-impact problems only.
- [DOC] Test table in English for the project page (no names, no images of children).

**Deliverable:** three fixes committed.
**Verification Gate:** each fix replayed by a member who did not make it. ✋ Confirm with the team.

---

## PHASE 15: 🧊 Feature Freeze
**Objective:** stop adding; start shipping.
**Depends on:** Phase 14 or the clock (Sun 12:30), whichever comes first.
**Tools**: F (the team log) · SH commit.

**Tasks:**
- Announce freeze; only bug and text fixes after this.

**Deliverable:** freeze commit.
**Verification Gate:** "Saat 12:30 oldu. Yeni özellik yok; sadece düzeltme. Onaylıyor musunuz?" ✋ Confirm with the team.

---

## PHASE 16: Final Package, Videos, Landing Page, Project Page
**Objective:** a judge who never runs the game still understands it completely.
**Depends on:** Phase 15.
**Time:** Sun 12:30-15:00.
**Tools**: SH `package.ps1`, `Get-FileHash` · S Sequencer + U Python Movie Render Queue · U `capture_image` · F landing page and `docs/submission.md` · SH git/gh · M OBS, paste, links.

**Tasks:**
- [BP] Final package; SHA-256 checksum; upload; size noted.
- [DOC] 30 s demo video (English subtitles; follows the guide: team, challenge, title and tagline, how it works with NASA data, future); screen capture only; no minors.
- [DOC] Full gameplay video, at most 4 minutes, English subtitles.
- [DOC] At least eight captioned screenshots.
- [DOC] Landing page (e.g. GitHub Pages): demo, full video, screenshots, download + "run in three steps", model card, teacher card, child-test summary, sources and licences, repo link, "Not endorsed by NASA".
- [DOC] Project page fields in English: summary, demo link, final project link, details, NASA data (DONKI, Mars Trek, OSDR if used, constants' sources), Space Agency Partner data (CSA food production), references, SI use.

**Deliverable:** all links ready.
**Verification Gate:** every link opened in a private window without login. ✋ Confirm with the team.

---

## PHASE 17: 🏁 Submit for Judging
**Objective:** submitted early, improved until the deadline.
**Depends on:** Phase 16.
**Time:** Sun 15:00 → jury; edits allowed until Sun 23:59.
**Tools**: M only (agent never submits).

**Tasks:**
- Tick "Terms and Conditions" and "Confirm All Work is Original"; press "Submit for Judging"; check the confirmation banner.
- Rehearse the spoken pitch (MISSING: jury time, length and language from the Local Lead).
- Any later fix: edit, save and resubmit before 23:59.

**Deliverable:** confirmation banner; certificate available.
**Verification Gate:** banner screenshot in the team log. ✋ Done.

---

## PHASE 18: Optional Moon Mode
**Objective:** show the Mars delay by contrast.
**Depends on:** Phase 13 green; must finish before Phase 15.
**Tools**: S `duplicate_asset`/`new_level`, lighting tools · V landscape · F Moon constants.

**Tasks:**
- [BP] `L_TD_MoonOutpost` duplicate with Moon terrain and light; lunar-night phase 5; Moon constants.

**Deliverable:** Moon mode selectable in Settings.
**Verification Gate:** one full Moon mission. ✋ Confirm with the team.

---

# APPENDIX: State-Machine Rules
1. One phase at a time per role. Finish and verify before starting the next.
2. Commit with Git after every phase that passes its gate.
3. If a phase is blocked, mark it `⏸️ BLOCKED`, write why in Notes, and ask the team.
4. Never change finished phases without writing the reason in Notes.
5. Cut order when late (first cut first): voice, hover preview, decision replay, music, Moon mode (Phase 18), two of the four optional science tasks.
6. Never cut: opening minute, three-system allocation, eight sourced cards, delayed advice, labelled DONKI panel, Mission Report, Source Panel, downloadable package, videos.
