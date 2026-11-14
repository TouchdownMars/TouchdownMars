# Touchdown Mars: First Light · Design Tokens & UI/UX System (design.md)

> **Source:** `docs/gdd.md` and the team research reports. Hex values, font and sizes are `PROPOSAL:` values; the team confirms them at the Sat 09:00 lock and after a color-blind simulator check. Status: **DRAFT**.
> **Aesthetic north stars:** NASA Perseverance surface imagery (the world); NASA mission-control flat displays (the decision layer); MISSING: one reference game chosen by the team.
> **Guiding law:** "A real Mars outside the window, a clear control panel under your hand."

---

## 1. Color Tokens

### 1.1 Core Palette
| Token | Hex | Use |
| :-- | :-- | :-- |
| `Core_Primary` | `#16243A` | Opaque bottom panel, card frames, Settings background |
| `Core_Secondary` | `#22344F` | Slots, buttons at rest, phase-strip background |
| `Core_Background` | `#0E1726` | Full-screen overlays (Mission Report, Source Panel) |
| `Core_Text` | `#F4F1EA` | All text on dark panels |
| `Core_Accent` | `#F7D046` | Power cells, the "Next phase" button, the ghost hand outline |

### 1.2 Character / Team Accents
| Token | Hex | Character | Use |
| :-- | :-- | :-- | :-- |
| `Sys_Air` | `#3A8DDE` | Flight Engineer | AIR slot, AIR light in scene, engineer stripe |
| `Sys_Greenhouse` | `#3FB27F` | Science Officer | GREENHOUSE slot, greenhouse light, science stripe |
| `Sys_Shelter` | `#F07F2E` | Health Officer | SHELTER slot, shelter light, health stripe |
| `Capcom_Frame` | `#C9D3E0` | CAPCOM | Portrait frame and Earth line |

### 1.3 Semantic Tokens
| Token | Value | Use |
| :-- | :-- | :-- |
| `Sem_Danger` | `#E5484D` + warning-triangle icon + flicker | System at level 1 or 0, alarm step |
| `Sem_Success` | `#3FB27F` + check icon | Stamped report line, system rising |
| `Sem_Disabled` | `#5B6577` at 60 % opacity + lock icon | "Next phase" before all cells are placed; locked Cadet system |
| `Sem_Low` | `#F2C14E` + half-fill icon | System at level 2 |
| `Sem_Live` | `#3FB27F` text "LIVE" + fetch time | DONKI value fetched this session |
| `Sem_Archive` | `#C9D3E0` text "ARCHIVE" + date | Any stored data |

## 2. Typography Tokens
| Token | Font / style | Use |
| :-- | :-- | :-- |
| `Type_Title` | Atkinson Hyperlegible Next, Bold | Phase name, card question, report title |
| `Type_Body` | Atkinson Hyperlegible Next, Regular | Card text, CAPCOM lines, Source Panel |
| `Type_Button` | Atkinson Hyperlegible Next, Bold, all caps for system words | Buttons, system words (AIR, GREENHOUSE, SHELTER) |
| Sizes | `T1 48` / `T2 36` / `Body 28` / `Caption 24` (pt at 1080p) | max 3 sizes per screen; nothing below 24 |

PROPOSAL: Atkinson Hyperlegible Next (SIL Open Font License, built for legibility). Check on 14 Nov that it has every Turkish glyph (ç ğ ı İ ö ş ü); if not, use Noto Sans for both languages. Record the font in `docs/sources.csv`.

## 3. Shape, Spacing & Material Tokens
| Token | Value | Note |
| :-- | :-- | :-- |
| `Space_Unit` | `8px` | all margins are multiples of this |
| `Panel_Padding` | `24px` | inside the bottom panel, cards and overlays |
| `Button_MinSize` | `96 x 96 px` (cells, slots); `240 x 72 px` (text buttons) | large targets for children |
| `Radius` | `16px` panels and cards; `48px` cells (pill) | soft, friendly corners |
| `Shadow` | `0 4px 12px #00000080` | only under the card and cells being dragged |
| `Panel_Height` | `33 %` of screen height | bottom third |
| `EnergyLine_Width` | `4px` per cell given, `2px` minimum | scene and panel lines |

## 4. Motion & Juice Tokens
| Token | Value | Use |
| :-- | :-- | :-- |
| `Motion_FocusBounce` | scale 1.00 → 1.06 → 1.00, 180 ms | hovered cell, slot or button |
| `Motion_PanelIn` | slide up 40 px + fade, 250 ms, ease-out | event card and overlays opening |
| `Motion_Confirm` | scale 1.00 → 0.94 → 1.00, 120 ms | click confirm |
| `Motion_CellPulse` | glow 0.6 → 1.0, 900 ms loop | the next valid cell during the opening and after 6-8 s idle |
| `Motion_CardFlip` | Y rotation 0 → 180°, 450 ms | "What really happened?" reveal |
| `Motion_SceneReact` | ≤ 1000 ms after click | lights, fan, crew reaction to a placement |
| Rule | every interaction = motion + sound | nothing responds silently; "Reduce motion" keeps sound and drops scale and camera moves |

## 5. UI Sound Tokens
| Token | Sound | Use |
| :-- | :-- | :-- |
| `SND_UI_Hover` | soft tick | focus move over a cell, slot or button |
| `SND_UI_Confirm` | short bright click | select, place cell, next phase |
| `SND_UI_Back` | low soft click | close card, close panel, take cell back |
| `SND_UI_Locked` | muted thud | press on a disabled button or locked slot |

## 6. Button & Focus State Spec
| State | Visual | Motion / Sound |
| :-- | :-- | :-- |
| Idle | `Core_Secondary` fill, `Core_Text` label, system color border on slots | none |
| Focused | border 4 px `Core_Accent`, slight brighten | `Motion_FocusBounce` + `SND_UI_Hover` |
| Pressed | fill darkened 15 % | `Motion_Confirm` + `SND_UI_Confirm` |
| Disabled | `Sem_Disabled`, lock icon | `SND_UI_Locked` on press, no motion |

## 7. Screen Inventory & Navigation Flow
```
[Boot] -> [Outpost scene + HUD (opening minute)] -> [Phase loop] -> [Mission Report (win)]
                     |                                   |      -> [Back to lander (lose)] -> [same seed restart]
                     +-> [Settings / Pause] <------------+
                     +-> [Source Panel] [Today's Sun] [Event Card]
```
| # | Screen | Key elements |
| :-- | :-- | :-- |
| S1 | Main Menu | None by design. Boot goes straight into S2. Corner group: gear, EN/TR, mute. |
| S2 | In-Game HUD | Phase strip, CAPCOM portrait and Earth line, power panel (cells, three slots, Next phase), Today's Sun badge, corner buttons, upcoming-event icon. |
| S3 | Pause | Resume, Restart same mission, New mission, Age band, Language, Sound, Quit. |
| S4 | Game Over | Scene with crew at lander, CAPCOM line naming the phase and system, "Change one thing and try again", "New mission". |
| S5 | Win | Mission Report: Crew, Base, Emergency, Bonus stamps; Minimum or Full mission; patch; seed; Fly again; New mission. |
| S6 | Settings | Language, Sound effects, Voice (if shipped), Age band, Window / Fullscreen, Reduce motion. |
| S7 | Event Card | Front: question + two choices + priority rule line. Back: date, instrument, measurement, label, source. Close button. |
| S8 | Source Panel | Table of every number: value, unit, source, URL, retrieved date, NASA measurement or designer value, LIVE or ARCHIVE. |
| S9 | Today's Sun | Flares / CMEs / storms of the last 30 days from DONKI, LIVE with fetch time or ARCHIVE with date, DONKI prototype disclaimer. |

## 8. HUD Spec
- **Phase strip:** top-left, six dots (five in Cadet), active dot large with phase name and sol range in `T2`; past dots filled, future dots outlined.
- **CAPCOM portrait:** top-right, 160 px portrait in `Capcom_Frame`, one line of text under it in `Body`; the line stays until the next message.
- **Earth message line:** under the portrait, a horizontal line from the outpost icon to an Earth icon; the message dot travels along it; on Mars the phase in which it was sent is stamped above the dot.
- **Power cells:** bottom panel left; 4 or 6 large cells; placed cells move into slots; the next valid cell pulses during the opening.
- **System slots:** bottom panel centre; three big slots (AIR, GREENHOUSE, SHELTER), each with icon, word, four-segment fill and cells placed; locked slot in Cadet shows a lock and "Good this phase".
- **Next phase button:** bottom panel right; disabled until all cells are placed; Space and Enter also work.
- **Upcoming-event icon:** above the slot the next event will hit, with a countdown mark (phases left).
- **Countdown marks:** above a system whose delayed result is pending (greenhouse growth).
- **Today's Sun badge:** bottom-left corner, small sun icon with LIVE or ARCHIVE tag; click opens S9.
- **Corner buttons:** bottom-right: Mute, EN/TR, "Where does this number come from?" (opens S8).
- **Dose badge:** Commander band only, beside SHELTER during radiation phases; counts storm exposures.
- **Ghost hand:** appears over the next valid move after 6-8 s without a purposeful click.

## 9. Naming Conventions
| Prefix | Asset type | Example |
| :-- | :-- | :-- |
| `BP_` | Blueprint actor | `BP_TD_AirUnit` |
| `WBP_` | UI widget | `WBP_TD_PowerPanel` |
| `M_` / `MI_` | Material / instance | `M_TD_SystemLight` / `MI_TD_SystemLight_Air` |
| `SM_` / `SK_` | Static / skeletal mesh | `SM_TD_Habitat` / `SK_TD_CrewSuit` |
| `T_` | Texture | `T_TD_Icon_Air` |
| `L_` | Level | `L_TD_Outpost` |
| `DT_` | DataTable | `DT_TD_Events` |
| `NS_` | Niagara system | `NS_TD_DustHaze` |
| `SFX_` | Sound | `SFX_TD_CardFlip` |
| `IA_` / `IMC_` | Input action / mapping context | `IA_TD_Click` / `IMC_TD_Default` |
| `E_` / `S_` | Blueprint enum / struct (only if not in C++) | `E_TD_CrewBehaviour` |

## 10. Folder Structure
```
Content/TouchdownMars/
  Blueprints/   BP_TD_* actors, GM_TD, PC_TD
  UI/           WBP_TD_*, icons, fonts
  Materials/    M_TD_*, MI_TD_*
  Meshes/       SM_TD_*, SK_TD_* (own and Tripo models)
  Maps/         L_TD_Test, L_TD_Outpost, L_TD_MoonOutpost
  Audio/        SFX_TD_*, music
  Data/         DT_TD_* (imported from /Data/Source/*.csv)
  VFX/          NS_TD_*
  NASA/         NASA 3D Resources models (listed in sources.csv)
Content/Fab/    Megascans and Fab imports (git-ignored)
```

## 11. Accessibility Rules
| Concern | Rule |
| :-- | :-- |
| Color-blind | Every system and state is shown by color + shape + icon + one word; never red/green alone. Check the palette with a deuteranopia simulator before Package 1. |
| Text scale | Body 28 pt at 1080p, nothing under 24 pt; UI scales with the DPI curve; text sits on opaque panels, never on the 3D scene. |
| Screen shake | None. Camera push-in and nudge switch off with "Reduce motion". |
| Photosensitivity | No flashes faster than 3 per second; storm alert is a slow light sweep. |
| Reading load | Cadet: one sentence per card, at most three technical terms per game. |
| Language | English default, Turkish selectable at any time; both complete. |
| Input | Left click only; large targets (96 px); keyboard shortcuts optional. |

## 12. Token Traceability
Every color, margin or motion value used in code or UMG carries a token comment, e.g. `/* Token: Sys_Air */`. New UI must not invent values that are not in this file. Add the token here first.

---

## Fill status
| Section | Status |
| :-- | :-- |
| 1 Color Tokens | FILLED (PROPOSAL values) |
| 2 Typography | FILLED (PROPOSAL font, glyph check pending) |
| 3 Shape & Spacing | FILLED |
| 4 Motion | FILLED |
| 5 UI Sound | FILLED |
| 6 Button States | FILLED |
| 7 Screens | FILLED |
| 8 HUD | FILLED |
| 9 Naming | FILLED |
| 10 Folders | FILLED |
| 11 Accessibility | FILLED |
| 12 Traceability | FILLED |
| Header north stars | reference game MISSING |


## Mission screens (design room and debrief), 14.11.2026 (NASA-punk revision)

Direction: NASA's own 1970s graphic language (1975 NASA Graphics Standards Manual, flight-hardware stencils, control-panel labels), the "NASA-punk" look players know from recent space games, used as a mood reference only (no game's logos, fonts or screens are copied). Research: `D:\NASA\research_notes\Görev tasarım oyunu GDC dersleri\ui_ux_onboarding.md` (NASA-STD-3001 Vol. 2 App. F, Into the Breach, Hodent, Xbox accessibility guidelines).

| Role | Hex | Use |
| :-- | :-- | :-- |
| Scene dim | `#080A0D` @ 55 % | behind the panels |
| Panel | `#EFEDE8` | panels |
| Raised | `#FAF9F6` | cards, guide strip |
| Well | `#E2DFD8` | value chips |
| Line | `#C8C4BA` | rules, idle borders, secondary buttons |
| Track | `#D9D5CC` | empty bar |
| Text | `#15181C` | main text |
| Text 2 | `#5C636E` | secondary text |
| Charcoal | `#22272E` | ordinary buttons (white text) |
| NASA red | `#E03C31` | primary actions only (Choose, Fix, Launch), section labels, the guide highlight (pulses at 0.8 Hz) |
| GO | `#1E8E4E` | passed, picked, GO |
| Caution | `#B7791F` | risk, below target |
| NO-GO | `#D7261E` | blocks launch, failed (always with a word, never colour alone) |

- **Fonts:** Barlow Semi Condensed (text; headings SemiBold with wide tracking) and IBM Plex Mono Medium (every number). SIL OFL 1.1, raw TTF files in `Content/TouchdownMars/UI/Fonts`, staged as non-UFS, loaded by `TeamFont()`. No automatic upper-casing (Turkish İ/I).
- **Pictures:** Google Gemini illustrations in one locked style: 1970s technical illustration and mission-poster look, flat matte shapes, light screen-print grain and halftone shadows, the UI palette (rust and ochre ground, cream sky, charcoal, off-white hardware, one red accent), low sun from the left, eye-level three-quarter view, the subject on Mars at our base. Never text, logos, insignia, flags, patches or faces (visors down). Each card shows the tag "AI illustration (Gemini)" (`ImageCredit`). Sources and crops: `D:\NASA\_kaynaklar\gemini_style` (cards 512 x 256, objectives 768 x 412, cold open 1024 x 512); the earlier NASA photos are kept in `D:\NASA\_kaynaklar\nasa_ui_photos_backup`.
- **Cards:** photo, name, value chips (mass violet, power orange, cost yellow, data blue, science green), one control; details in the hover tooltip.
- **Status:** colour + bar + word. Bars ease to the new value in about 300 ms.
- **Guide (flight 2+):** warnings only: shown when a line is red; "Start from a working design" loads a design that launches.

### First flight (onboarding) look
Rules and steps: `gdd.md` §1 Main Mechanics 13.
- **Step strip** (Raised panel under the header): red badge `n/7` (IBM Plex Mono), a red `FLIGHT` tag with the one-line cause of the step just finished (Text 2, 14 pt), the instruction (Text, 19 pt bold), the causal map, and the actions on the right (prediction buttons in charcoal; fix choices in NASA red when they solve the line, charcoal when they are "not enough alone").
- **Causal map:** chips 11 pt bold. Unmet: Track fill, Text 2. Met: chain colour with dark text (science blue `#3A8DDE`, cargo violet `#B58CF0`, power orange `#F07F2E`, GO red). Failing: NO-GO fill, white text. Current: pulsing red outline. Arrows in IBM Plex Mono (Barlow has no arrow glyphs).
- **Power cells:** square tokens (16-26 px), GO green when on, NO-GO at 28 % for a missing required cell, Track for spare slots; a 3 px charcoal line after the required count. Rows: clear sky and dust storm. The cold open and the storm preview use the same tokens.
- **Mission calendar:** 31 sol squares at the top of the dashboard; orange = dust storm possible, amber = solar storm possible, grey = breakdown, charcoal = no link; a legend under it, never colour alone.
- **Locked card:** photo washed out, charcoal `LOCKED` tag, disabled button with the reason ("Unlocks when you need it"). Flight 2: red `NEW` tag, the reason in the tooltip.
- **Cold open:** charcoal panel (inverse of the light room), red `REPLAY` tag, Gemini illustration of the Ares-0 outpost darkening under a dust tint, cells draining from 8 to 1; white text, warm orange for the storm line, light red for the failure.
- **Poll the room:** the four role reports appear one by one (0.75 s apart) under the poll buttons in IBM Plex Mono, each with a GO chip (amber "GO · goal open" when the role goal is not met).
- **Debrief question:** a panel with a pulsing red outline until the player picks; the right decision turns GO green, a wrong pick NO-GO; then the trace in IBM Plex Mono and the causal map with the path in red.
