# Observer notes

Two morning sessions were screen-recorded: the game window only, with no camera, no microphone and no names. The recordings are not published; these notes are the only part used. They were coded from frames every 10 seconds, and at 1 to 5 frames per second around the key moments. The meaning of the debrief colours and messages was checked in the game source (`Source/TouchdownMars/UI/TDDesignRoom.cpp`, `Data/Source/text.csv`).

## P06 (15 Nov 2026, 08:38 to 08:59, Turkish interface, 21 min, played alone)

- First Crew, first flight, guided design room. Stayed on guide step 1 of 7 for about 50 seconds and visited three consoles, then used "Show me a working design" (design code `FH2-KP3-SB-GS-MZ-DL`). This skipped the guide's data prediction step and its relay-or-direct-link step.
- Launched about one minute after the first screen. No blocked launch was seen.
- Each phase: Mission Control (power cells, then the crew plan, giving tasks to the crew characters), then work on the surface. Four event decisions; the debrief says all four kept every system safe.
- Debrief: mission complete, 94/100 (mission 100, science 80, efficiency 86, decisions 100), one flight this session. The Science Lead and Comms and Budget goals stayed open.
- "Which decision would you change?": after about 13 seconds picked Power units. The debrief then traced the shortfall to the link to Earth: 120 Mb/sol made, 60 Mb/sol link, no link on sols 21 to 27, 39 % of the science data reached Earth.
- The recording ends on the debrief; no redesign in the recording.
- Form: said the mission succeeded and that the power screen did not show the remaining power; did not name the link.

## P08 (15 Nov 2026, 09:04 to 09:14, English interface, 10 min recorded, played alone)

- The recording starts on the same working design; launched within 10 seconds.
- The first Mission Control visit took 4 minutes (P06: 1.5 minutes): about 40 seconds on the power screen before placing a cell, about 40 seconds on the Decision tab when no event was waiting, and several returns to Crew Plan and Sun + Sources.
- Sols 4 and 5: the systems needed 8 power cells and 6 were available; Air got 2 of 3 and dropped a level.
- Quit from the pause menu on sols 7 to 10, so no debrief and no second flight.
- Form: left the open questions empty. `s3_debrief_cause` and `s5_refly_taught` are not counted (see `README.md`, coding rules).

## What we change

- The power screen should show the total need next to the cells available.
- The working-design shortcut should not skip the data link step that the debrief asks about later.
- Mission Control tabs: make the next required tab obvious; the Decision tab is opened when nothing is waiting.
