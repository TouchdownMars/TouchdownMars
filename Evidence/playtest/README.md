# Play test with people outside the team

People who are not on the team played the game and then answered a short form. This folder holds their anonymous answers, how the test was run, the results, and the limits of what the results show.

**What this test can show:** whether players understand what to do, where they get stuck, and whether they can say what went wrong in their own mission.

**What it cannot show:** a learning effect. Nothing was measured before play, there is no comparison group, and the ratings are the players' own view. No learning claim is made.

## Results

<!-- results:start -->
_Written by `Scripts/playtest_report.py` from 8 form responses. To change a result, edit `coding.csv` and run the script again._

**Players counted: 8.** Every counted player ticked the 18+ and consent box.
Mistyped player number corrected by the observer: typed `1810`, counted as P04; typed `6` (form row 9, the same number as another player), counted as P08.
Answers kept in `responses.csv` but not counted (reason in `coding.csv`): P08: `s3_debrief_cause`, `s5_refly_taught`.

**Knowledge of space mission engineering before playing:** Nothing 1/8, A little 6/8, A lot 1/8.

**Agreement with six statements** (1 strongly disagree, 5 strongly agree; number of players per answer):

| Statement | 1 | 2 | 3 | 4 | 5 | Median | Agree (4-5) |
| :-- | --: | --: | --: | --: | --: | --: | --: |
| `s1_understood` I understood what I had to do without outside help. | 0 | 0 | 1 | 5 | 2 | 4 | 7/8 |
| `s2_tradeoffs` The design room made the trade-offs between mass, power, budget and data clear. | 1 | 0 | 0 | 4 | 3 | 4 | 7/8 |
| `s3_debrief_cause` The debrief showed me which design decision caused my main problem. | 0 | 1 | 1 | 3 | 2 | 4 | 5/7 |
| `s4_nasa_vs_game` I could tell which numbers come from NASA sources and which are game values. | 0 | 1 | 3 | 1 | 3 | 3.5 | 4/8 |
| `s5_refly_taught` Flying the same mission again with a changed design taught me something new. | 1 | 0 | 1 | 0 | 5 | 5 | 5/7 |
| `s6_play_again` I would like to play again. | 0 | 0 | 1 | 3 | 4 | 4.5 | 7/8 |

**Coded answers and sessions:**

| Measure | Result |
| :-- | :-- |
| Dust storm question, solar against fission (`dust_storm`; asked after play only) | correct 4/8, partial 1/8, wrong 2/8, no_answer 1/8 |
| "What went wrong" answer against the player's own run record (`cause_match`) | match 0/8, partial 0/8, no_match 0/8, na 8/8 |
| Model | not recorded |
| Solo or team | solo 2/2, team 0/2 (6 of 8 not recorded) |
| Minutes played | median 15.5, range 10 to 21 (2 of 8 players) |
| Run records in `runs/` | none matched yet |
<!-- results:end -->

Counts and medians only. With this few players, averages and significance tests would say more than the data can.

## Method

| | |
| :-- | :-- |
| Date | 15 November 2026, two sittings (night: forms sent 00:12 to 02:29; morning: 08:38 to 09:26) |
| Players, and how they were found | 8 adults from outside the team: professionals who came by on their own and asked to play. Nobody was recruited or selected. The place was not recorded |
| How they played | alone; on the surface each player gives the crew's tasks to the game's crew characters |
| Observer | a team member, present while the player played |
| Screen recordings | two morning sessions (P06, P08): the game window only, no camera, no microphone. The recordings are not published, because the consent covers the anonymous answers and the observer notes, not video. What they show is in `observer_notes.md` |
| Game log and run records | not saved during the test because of a recording error. No player has a run record, so `cause_match` is `na` for every player |
| Form | "Touchdown Mars: Playtest", Google Forms, in English, filled in after play; the link is not published so that no one outside the test can add answers |
| Questions | player number, age and consent box, knowledge before playing (Nothing, A little, A lot), six statements on a 1 to 5 agreement scale, four open questions |
| Player number | given by the observer; the form asks for no name and no contact details |
| Who is not counted | team members and anyone under 18 |
| Consent | every player ticked a required box: 18 or older, privacy notice read, explicit consent to processing and to publication of the anonymous answers |
| Raw data | kept in Google Forms; deleted after export and no later than 30 November 2026 |

## Files

| File | What it holds |
| :-- | :-- |
| `responses.csv` | One row per counted player, UTF-8. Written by `Scripts/playtest_report.py`. Open answers are copied as typed. Timestamps and the consent text are left out. |
| `coding.csv` | Observer and coder sheet, one row per player, filled in by hand: session facts, the two codes, translations, and any player number or exclusion decision. `form_row` is set when two players typed the same number; `not_counted` lists statements whose answer is kept but not counted, with the reason in `note`. |
| `observer_notes.md` | What the two screen recordings show, one section per player, no names. |
| `runs/` | Run records copied from `Saved/Runs/` after each flight and renamed by player and flight, for example `P03_flight1.json`. |

To rebuild after new answers: download the responses from Google Forms, then `python Scripts/playtest_report.py <export.csv or .zip>`.

### Columns of `responses.csv`

| Column | Meaning |
| :-- | :-- |
| `player` | P01, P02, ... in the order the observer gave the numbers |
| `prior_knowledge` | Nothing, A little or A lot |
| `s1_understood` to `s6_play_again` | agreement with the six statements in the results table, 1 (strongly disagree) to 5 (strongly agree) |
| `went_wrong` | "What went wrong in your mission, and which design decision do you think caused it?" |
| `change_second_flight` | "What did you change (or what would you change) for the second flight, and why?" |
| `dust_storm_answer` | "In a dust storm, what is the difference between solar power and fission power?" |
| `confused_most` | "What confused you most?" |
| `..._en` | English translation of an open answer written in another language; present only when one exists |
| `model`, `group`, `minutes` | Guided or Advanced; solo or team; minutes played (observer) |
| `flights`, `run_results`, `run_causes` | from the player's run records: number of flights, the result of each flight, and the problem cause keys of each flight (flights separated by `;`, causes by `+`) |
| `dust_storm` | code for `dust_storm_answer`: correct, partial, wrong or no_answer |
| `cause_match` | code for `went_wrong` against `run_causes`: match, partial, no_match or na |
| `not_counted` | statements answered but not counted for this player (reason in `coding.csv`) |

## Coding rules

**`dust_storm`.** In the game, solar power falls in a dust-storm phase and fission power does not.
- `correct`: the answer says both: solar gives less or stops, and fission keeps working.
- `partial`: one of the two, stated correctly, and nothing wrong.
- `wrong`: neither, a wrong statement, or "I don't know".
- `no_answer`: the question was optional and left empty.

**`cause_match`.** The `went_wrong` answer is compared with the problem cause keys in the same player's run records (`run_causes`). The cause key covers power shortfalls of AIR, GREENHOUSE and SHELTER, storm exposure, and air running out.
- `match`: the answer names a system or problem the record shows, or the choice behind it (where the power cells went, too little power in the design, the shelter).
- `partial`: the answer points at the right area but not at a system or choice the record shows.
- `no_match`: the answer names a problem the record does not show.
- `na`: the answer is not about a mission problem (menus, controls), or the player has no run record.

**`not_counted`.** A statement is not counted for a player whose session did not include what it asks about. P08 quit on sols 7 to 10 (screen recording), so P08 saw no debrief and flew once: `s3_debrief_cause` and `s5_refly_taught` are kept in `responses.csv` and left out of the counts. This lowers both results.

The `dust_storm` codes were assigned with the rule above by an AI assistant (Claude) and checked by the team.

The debrief also asks the player "Which decision would you change?" before it shows the cause. One player was seen answering it (P06, `observer_notes.md`). This is an observation of one session, not a `cause_match` code.

## Limits

- **Few players, self-selected.** Eight professionals who came by and asked to play. They are likely more interested in the game than a general audience, and the numbers describe these players only.
- **No run records.** The game log and run records were not saved during the test, so no answer could be checked against what happened in the player's own flight.
- **Two observed sessions.** Only two sessions were screen-recorded, screen only, with no audio.
- **One flight for most players.** Only P01 wrote that they flew a second time; P06 flew once and P08 did not finish. Read `s5_refly_taught` with this in mind.
- **Crew tasks are not a debrief cause.** Two players named giving tasks to the crew as the cause of their problem; the debrief causes cover the design decisions, not crew tasks.
- **Run by the team.** A team member observed each session, which can make answers more polite.
- **After play only, self-reported.** The six ratings are the players' own judgement. The dust storm question is the only knowledge check, and it was asked once, after play.
- **English form.** The form and the open questions were in English only.
- **Self-declared.** Age is what the player ticked. The player typed the player number; a mistyped number is corrected by the observer and listed in the results.
