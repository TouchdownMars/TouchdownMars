# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Turn the Google Forms export of the play test into the public evidence files.

Runs outside Unreal with Python 3.9+ and the standard library only:
    python Scripts/playtest_report.py "Touchdown Mars_ Playtest.csv.zip"
    python Scripts/playtest_report.py export.csv --fix-player 1810=4
    python Scripts/playtest_report.py export.csv --fix-row 9=8

Reads
    the Forms export (.csv, or the .zip that "Download responses" gives)
    Evidence/playtest/coding.csv              observer and coder sheet, filled in by hand
    Evidence/playtest/runs/P03_flight1.json   run records copied from Saved/Runs/, renamed per player
Writes
    Evidence/playtest/responses.csv   one row per counted player; no timestamp, no consent text
    Evidence/playtest/coding.csv      one row per player; new players are added, filled cells are kept
    Evidence/playtest/README.md       only the block between the results markers is replaced
Only the form columns named below are copied. Any other column (an e-mail address, for example) is dropped.
Nothing is written unless every input passes its checks.
"""

from __future__ import annotations

import argparse
import csv
import io
import json
import re
import statistics
import sys
import zipfile
from pathlib import Path

MAX_PLAYER = 99
CONSENT_PREFIX = "I am 18 or older"

PLAYER_HEADER = "Player number (the observer will tell you)"
CONSENT_HEADER = "Age confirmation and consent"
PRIOR_HEADER = "How much did you know about space mission engineering before playing?"
PRIOR_LEVELS = ("Nothing", "A little", "A lot")

# Agreement grid: statement text inside the square brackets of the form header -> column name.
LIKERT = {
    "I understood what I had to do without outside help.": "s1_understood",
    "The design room made the trade-offs between mass, power, budget and data clear.": "s2_tradeoffs",
    "The debrief showed me which design decision caused my main problem.": "s3_debrief_cause",
    "I could tell which numbers come from NASA sources and which are game values.": "s4_nasa_vs_game",
    "Flying the same mission again with a changed design taught me something new.": "s5_refly_taught",
    "I would like to play again.": "s6_play_again",
}
# Open questions: form header -> column name.
OPEN = {
    "What went wrong in your mission, and which design decision do you think caused it?": "went_wrong",
    "What did you change (or what would you change) for the second flight, and why?": "change_second_flight",
    "In a dust storm, what is the difference between solar power and fission power?": "dust_storm_answer",
    "What confused you most?": "confused_most",
}
EN_FIELDS = tuple(f"{name}_en" for name in OPEN.values())

CODING_FIELDS = (
    "player", "form_player", "form_row", "exclude_reason", "model", "group", "minutes",
    "dust_storm", "cause_match", "not_counted", *EN_FIELDS, "note",
)
CODES = {
    "model": ("Guided", "Advanced", "mixed"),
    "group": ("solo", "team"),
    "dust_storm": ("correct", "partial", "wrong", "no_answer"),
    "cause_match": ("match", "partial", "no_match", "na"),
}

GRID_HEADER = re.compile(r"\[(.+)\]$")
RUN_FILE = re.compile(r"^P(\d{1,2})_flight(\d+)\.json$", re.IGNORECASE)
RESULTS_START = "<!-- results:start -->"
RESULTS_END = "<!-- results:end -->"
RESULTS_BLOCK = re.compile(re.escape(RESULTS_START) + r".*?" + re.escape(RESULTS_END), re.DOTALL)


class ReportError(Exception):
    """A problem in the input that a person has to resolve."""


def load_text(path: Path) -> str:
    """Return the CSV text of a .csv file or of the single .csv inside a .zip."""
    try:
        if path.suffix.lower() == ".zip":
            with zipfile.ZipFile(path) as archive:
                members = [name for name in archive.namelist() if name.lower().endswith(".csv")]
                if len(members) != 1:
                    raise ReportError(f"{path.name}: expected one .csv inside, found {len(members)}")
                data = archive.read(members[0])
        else:
            data = path.read_bytes()
    except FileNotFoundError:
        raise ReportError(f"{path}: file not found") from None
    except zipfile.BadZipFile:
        raise ReportError(f"{path.name}: not a readable .zip file") from None
    try:
        return data.decode("utf-8-sig")
    except UnicodeDecodeError:
        raise ReportError(f"{path.name}: not UTF-8. Export it again, or save it as 'CSV UTF-8'.") from None


def read_table(text: str, source: str) -> tuple[list[str], list[dict[str, str]]]:
    """Parse CSV text into headers and rows. Accepts the semicolon files Excel writes in some locales."""
    first_line = text.split("\n", 1)[0]
    delimiter = ";" if first_line.count(";") > first_line.count(",") else ","
    reader = csv.reader(io.StringIO(text, newline=""), delimiter=delimiter)
    try:
        table = [cells for cells in reader if any(cell.strip() for cell in cells)]
    except csv.Error as error:
        raise ReportError(f"{source}: {error}") from None
    if not table:
        raise ReportError(f"{source}: the file is empty")
    headers = [" ".join(cell.split()) for cell in table[0]]
    repeated = sorted({header for header in headers if headers.count(header) > 1})
    if repeated:
        raise ReportError(f"{source}: column appears more than once: {', '.join(repeated)}")
    rows = []
    for cells in table[1:]:
        padded = cells + [""] * (len(headers) - len(cells))
        rows.append({header: padded[index].strip() for index, header in enumerate(headers)})
    return headers, rows


def parse_player(value: str, where: str, hint: str = "") -> int:
    """Read a player number written as 4, 04 or P04."""
    match = re.fullmatch(r"[Pp]?0*(\d{1,2})", value)
    if not match or not 1 <= int(match.group(1)) <= MAX_PLAYER:
        raise ReportError(f"{where}: player number {value!r} is not between 1 and {MAX_PLAYER}.{hint}")
    return int(match.group(1))


def column_map(headers: list[str], source: str) -> tuple[dict[str, str], list[str]]:
    """Map output column names to form headers. Returns the map and the form headers that are not copied."""
    wanted = {PLAYER_HEADER: "player", CONSENT_HEADER: "consent", PRIOR_HEADER: "prior_knowledge", **OPEN}
    mapping: dict[str, str] = {}
    dropped: list[str] = []
    for header in headers:
        grid = GRID_HEADER.search(header)
        if header in wanted:
            mapping[wanted[header]] = header
        elif grid and grid.group(1) in LIKERT:
            mapping[LIKERT[grid.group(1)]] = header
        else:
            dropped.append(header)
    missing = [name for name in (*wanted.values(), *LIKERT.values()) if name not in mapping]
    if missing:
        raise ReportError(
            f"{source}: no form column found for {', '.join(missing)}. "
            "If a question was reworded in the form, update the header text at the top of this script."
        )
    return mapping, dropped


def read_coding(path: Path) -> dict[int, dict[str, str]]:
    """Read the observer and coder sheet. A missing file is an empty sheet."""
    if not path.exists():
        return {}
    headers, rows = read_table(load_text(path), path.name)
    unknown = [header for header in headers if header not in CODING_FIELDS]
    if unknown:
        raise ReportError(f"{path.name}: unknown column {', '.join(unknown)}. Allowed: {', '.join(CODING_FIELDS)}")
    if "player" not in headers:
        raise ReportError(f"{path.name}: the 'player' column is missing")
    sheet: dict[int, dict[str, str]] = {}
    for line, row in enumerate(rows, start=2):
        where = f"{path.name} row {line}"
        player = parse_player(row["player"], where)
        if player in sheet:
            raise ReportError(f"{where}: P{player:02d} has more than one row")
        entry = {field: row.get(field, "") for field in CODING_FIELDS}
        for field, allowed in CODES.items():
            if entry[field] and entry[field] not in allowed:
                raise ReportError(f"{where}: {field} is {entry[field]!r}; use one of {', '.join(allowed)} or leave it empty")
        if entry["minutes"] and not entry["minutes"].isdigit():
            raise ReportError(f"{where}: minutes is {entry['minutes']!r}; use a whole number or leave it empty")
        if entry["form_row"] and not entry["form_row"].isdigit():
            raise ReportError(f"{where}: form_row is {entry['form_row']!r}; use the row number of the export or leave it empty")
        unknown_items = [item for item in entry["not_counted"].split(";") if item and item not in LIKERT.values()]
        if unknown_items:
            raise ReportError(f"{where}: not_counted names {', '.join(unknown_items)}; use statement columns such as s3_debrief_cause")
        sheet[player] = entry
    return sheet


def player_fixes(coding: dict[int, dict[str, str]], arguments: list[str]) -> dict[str, int]:
    """Map a mistyped form player number to the assigned one: stored in coding.csv, or given with --fix-player."""
    # A row fix also stores the typed number; that number belongs to another player, so it is not a typed-number fix.
    fixes = {entry["form_player"]: player for player, entry in coding.items() if entry["form_player"] and not entry["form_row"]}
    for argument in arguments:
        typed, separator, assigned = argument.partition("=")
        typed = typed.strip()
        if not separator or not typed:
            raise ReportError(f"--fix-player {argument}: write it as TYPED=ASSIGNED, for example 1810=4")
        player = parse_player(assigned.strip(), f"--fix-player {argument}")
        if fixes.get(typed, player) != player:
            raise ReportError(f"--fix-player {argument}: coding.csv already maps {typed} to P{fixes[typed]:02d}")
        fixes[typed] = player
    return fixes


def row_fixes(coding: dict[int, dict[str, str]], arguments: list[str]) -> dict[int, int]:
    """Map an export row to a player, for two players who typed the same number: stored in coding.csv, or given with --fix-row."""
    fixes = {int(entry["form_row"]): player for player, entry in coding.items() if entry["form_row"]}
    for argument in arguments:
        row, separator, assigned = argument.partition("=")
        if not separator or not row.strip().isdigit():
            raise ReportError(f"--fix-row {argument}: write it as ROW=ASSIGNED, for example 9=8 (row 1 is the header)")
        player = parse_player(assigned.strip(), f"--fix-row {argument}")
        if fixes.get(int(row), player) != player:
            raise ReportError(f"--fix-row {argument}: coding.csv already maps row {row} to P{fixes[int(row)]:02d}")
        fixes[int(row)] = player
    return fixes


def read_runs(runs_dir: Path) -> tuple[dict[int, list[dict]], list[str]]:
    """Read the run records in runs/. Returns flights per player (in flight order) and the .json names ignored."""
    flights: dict[int, list[dict]] = {}
    ignored: list[str] = []
    if not runs_dir.is_dir():
        return flights, ignored
    for path in sorted(runs_dir.iterdir()):
        match = RUN_FILE.match(path.name)
        if not match:
            if path.suffix.lower() == ".json":
                ignored.append(path.name)
            continue
        try:
            record = json.loads(path.read_text(encoding="utf-8-sig"))
            causes = [phase["cause_key"] for phase in record["phases"]]
            flight = {"index": int(match.group(2)), "model": record["model"], "result": record["result"]}
        except (UnicodeDecodeError, json.JSONDecodeError) as error:
            raise ReportError(f"runs/{path.name}: not readable JSON ({error})") from None
        except (KeyError, TypeError) as error:
            raise ReportError(f"runs/{path.name}: not a run record (missing {error})") from None
        # Only causes that name a problem; "<System>_Up" and "Steady" phases went well.
        problems = [key.removeprefix("Cause_") for key in causes if key and key != "Cause_Steady" and not key.endswith("_Up")]
        flight["causes"] = list(dict.fromkeys(problems))
        flights.setdefault(int(match.group(1)), []).append(flight)
    for player_flights in flights.values():
        player_flights.sort(key=lambda flight: flight["index"])
    return flights, ignored


def likert(value: str, where: str) -> int | str:
    """'4 Agree' -> 4. An unanswered grid row stays empty."""
    if not value:
        return ""
    match = re.match(r"([1-5])(?!\d)", value)
    if not match:
        raise ReportError(f"{where}: expected an answer that starts with 1 to 5, got {value!r}")
    return int(match.group(1))


def session_model(player: int, coded: str, flights: list[dict]) -> str:
    """Model from the coder sheet, checked against the run records; from the run records when the sheet is empty."""
    flown = sorted({flight["model"] for flight in flights})
    if coded and coded != "mixed" and flown and flown != [coded]:
        raise ReportError(f"P{player:02d}: coding.csv says model {coded}, the run records say {', '.join(flown)}")
    if coded or not flown:
        return coded
    return flown[0] if len(flown) == 1 else "mixed"


def resolve_player(line: int, typed: str, fixes: dict[str, int], rows_fixed: dict[int, int]) -> int:
    """The counted player of one export row: a row fix first, then a typed-number fix, then the typed number."""
    if line in rows_fixed:
        return rows_fixed[line]
    if typed in fixes:
        return fixes[typed]
    hint = f" If it is a typing mistake, run again with --fix-player {typed}=<assigned number>."
    return parse_player(typed, f"form row {line}", hint)


def build(export_rows, mapping, coding, flights, fixes, rows_fixed):
    """Check every form row. Returns counted rows, excluded players, rows without consent and the coder sheet to save."""
    sheet = {player: dict(entry) for player, entry in coding.items()}
    counted: dict[int, dict] = {}
    excluded: list[tuple[int, str]] = []
    form_row_of: dict[int, int] = {}
    no_consent = 0
    for line, raw in enumerate(export_rows, start=2):
        where = f"form row {line}"
        if not raw[mapping["consent"]].startswith(CONSENT_PREFIX):
            no_consent += 1
            continue
        typed = raw[mapping["player"]]
        player = resolve_player(line, typed, fixes, rows_fixed)
        if player in form_row_of:
            raise ReportError(
                f"{where}: P{player:02d} is already in form row {form_row_of[player]}. "
                "If one of the two typed a wrong number, use --fix-player TYPED=ASSIGNED, "
                "or --fix-row ROW=ASSIGNED when both typed the same number."
            )
        form_row_of[player] = line
        entry = sheet.setdefault(player, dict.fromkeys(CODING_FIELDS, ""))
        entry["player"] = f"P{player:02d}"
        if line in rows_fixed:
            entry["form_row"] = str(line)
        if typed in fixes or line in rows_fixed:
            entry["form_player"] = typed
        if entry["exclude_reason"]:
            excluded.append((player, entry["exclude_reason"]))
            continue
        prior = raw[mapping["prior_knowledge"]]
        if prior and prior not in PRIOR_LEVELS:
            raise ReportError(f"{where}: prior knowledge is {prior!r}; the form offers {', '.join(PRIOR_LEVELS)}")
        row = {"player": entry["player"], "prior_knowledge": prior}
        for column in LIKERT.values():
            row[column] = likert(raw[mapping[column]], f"{where}, {column}")
        for column in OPEN.values():
            row[column] = raw[mapping[column]]
        for column in EN_FIELDS:
            row[column] = entry[column]
        flown = flights.get(player, [])
        row["model"] = session_model(player, entry["model"], flown)
        row["group"] = entry["group"]
        row["minutes"] = entry["minutes"]
        row["flights"] = len(flown) if flown else ""
        row["run_results"] = ";".join(flight["result"] for flight in flown)
        row["run_causes"] = ";".join("+".join(flight["causes"]) or "none" for flight in flown)
        row["dust_storm"] = entry["dust_storm"]
        row["cause_match"] = entry["cause_match"]
        row["not_counted"] = entry["not_counted"]
        counted[player] = row
    for player, entry in sheet.items():
        entry["player"] = f"P{player:02d}"
    rows = [counted[player] for player in sorted(counted)]
    return rows, sorted(excluded), no_consent, [sheet[player] for player in sorted(sheet)]


def tally(values: list[str], order: tuple[str, ...], missing: str) -> str:
    """'correct 2/5, partial 1/5, wrong 2/5', with the number of empty cells when there are any."""
    filled = [value for value in values if value]
    if not filled:
        return missing
    text = ", ".join(f"{label} {filled.count(label)}/{len(filled)}" for label in order)
    if len(filled) < len(values):
        text += f" ({len(values) - len(filled)} of {len(values)} {missing})"
    return text


def results_block(rows, excluded, no_consent, form_rows, sheet) -> str:
    """The Markdown block that goes between the results markers of Evidence/playtest/README.md."""
    count = len(rows)
    lines = [
        RESULTS_START,
        f"_Written by `Scripts/playtest_report.py` from {form_rows} form responses. "
        "To change a result, edit `coding.csv` and run the script again._",
        "",
        f"**Players counted: {count}.** Every counted player ticked the 18+ and consent box.",
    ]
    if excluded:
        reasons = "; ".join(f"P{player:02d}: {reason}" for player, reason in excluded)
        lines.append(f"Not counted: {len(excluded)} ({reasons}).")
    if no_consent:
        lines.append(f"Form rows without the consent box, not used and not published: {no_consent}.")
    corrected = [
        f"typed `{entry['form_player']}`"
        + (f" (form row {entry['form_row']}, the same number as another player)" if entry["form_row"] else "")
        + f", counted as {entry['player']}"
        for entry in sheet if entry["form_player"]
    ]
    if corrected:
        lines.append(f"Mistyped player number corrected by the observer: {'; '.join(corrected)}.")
    skipped = [
        row["player"] + ": " + ", ".join(f"`{item}`" for item in row["not_counted"].split(";"))
        for row in rows if row["not_counted"]
    ]
    if skipped:
        lines.append(f"Answers kept in `responses.csv` but not counted (reason in `coding.csv`): {'; '.join(skipped)}.")
    lines += [
        "",
        "**Knowledge of space mission engineering before playing:** "
        + tally([row["prior_knowledge"] for row in rows], PRIOR_LEVELS, "not answered") + ".",
        "",
        "**Agreement with six statements** (1 strongly disagree, 5 strongly agree; number of players per answer):",
        "",
        "| Statement | 1 | 2 | 3 | 4 | 5 | Median | Agree (4-5) |",
        "| :-- | --: | --: | --: | --: | --: | --: | --: |",
    ]
    for statement, column in LIKERT.items():
        answers = [row[column] for row in rows if row[column] != "" and column not in row["not_counted"].split(";")]
        cells = " | ".join(str(answers.count(score)) for score in range(1, 6))
        median = f"{statistics.median(answers):g}" if answers else "-"
        agree = sum(1 for answer in answers if answer >= 4)
        lines.append(f"| `{column}` {statement} | {cells} | {median} | {agree}/{len(answers)} |")
    minutes = [int(row["minutes"]) for row in rows if row["minutes"]]
    if minutes:
        played = f"median {statistics.median(minutes):g}, range {min(minutes)} to {max(minutes)} ({len(minutes)} of {count} players)"
    else:
        played = "not recorded"
    models = [row["model"] for row in rows]
    with_records = [row for row in rows if row["flights"] != ""]
    if with_records:
        total = sum(row["flights"] for row in with_records)
        records = f"{len(with_records)} of {count} players, {total} flights"
    else:
        records = "none matched yet"
    lines += [
        "",
        "**Coded answers and sessions:**",
        "",
        "| Measure | Result |",
        "| :-- | :-- |",
        "| Dust storm question, solar against fission (`dust_storm`; asked after play only) | "
        + tally([row["dust_storm"] for row in rows], CODES["dust_storm"], "not coded yet") + " |",
        "| \"What went wrong\" answer against the player's own run record (`cause_match`) | "
        + tally([row["cause_match"] for row in rows], CODES["cause_match"], "not coded yet") + " |",
        "| Model | " + tally(models, tuple(label for label in CODES["model"] if label != "mixed" or label in models), "not recorded") + " |",
        "| Solo or team | " + tally([row["group"] for row in rows], CODES["group"], "not recorded") + " |",
        f"| Minutes played | {played} |",
        f"| Run records in `runs/` | {records} |",
        RESULTS_END,
    ]
    return "\n".join(lines)


def write_csv(path: Path, fields: tuple[str, ...] | list[str], rows: list[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)


def run(export: Path, out: Path, fix_arguments: list[str], row_arguments: list[str]) -> None:
    headers, export_rows = read_table(load_text(export), export.name)
    mapping, dropped = column_map(headers, export.name)
    coding = read_coding(out / "coding.csv")
    fixes = player_fixes(coding, fix_arguments)
    rows_fixed = row_fixes(coding, row_arguments)
    flights, ignored = read_runs(out / "runs")
    rows, excluded, no_consent, sheet = build(export_rows, mapping, coding, flights, fixes, rows_fixed)
    block = results_block(rows, excluded, no_consent, len(export_rows), sheet)

    readme = out / "README.md"
    readme_text = None
    if readme.exists():
        readme_text = readme.read_text(encoding="utf-8")
        if len(RESULTS_BLOCK.findall(readme_text)) != 1:
            raise ReportError(f"{readme}: expected one block between {RESULTS_START} and {RESULTS_END}")

    # The translation columns appear only when at least one answer has a translation.
    en_fields = [field for field in EN_FIELDS if any(row[field] for row in rows)]
    fields = [
        "player", "prior_knowledge", *LIKERT.values(), *OPEN.values(), *en_fields,
        "model", "group", "minutes", "flights", "run_results", "run_causes", "dust_storm", "cause_match",
        *(["not_counted"] if any(row["not_counted"] for row in rows) else []),
    ]
    out.mkdir(parents=True, exist_ok=True)
    write_csv(out / "responses.csv", fields, [{field: row[field] for field in fields} for row in rows])
    write_csv(out / "coding.csv", CODING_FIELDS, sheet)
    if readme_text is not None:
        with readme.open("w", encoding="utf-8", newline="\n") as handle:
            handle.write(RESULTS_BLOCK.sub(lambda _: block, readme_text))

    print(block)
    print()
    print(f"responses.csv: {len(rows)} players. coding.csv: {len(sheet)} rows.")
    print(f"Form columns not copied: {', '.join(dropped + [CONSENT_HEADER])}")
    if ignored:
        print(f"runs/: ignored (name is not like P03_flight1.json): {', '.join(ignored)}")
    if readme_text is None:
        print(f"{readme} does not exist; the results block above was not saved.")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build Evidence/playtest/ from the Google Forms export.")
    parser.add_argument("export", type=Path, help="the Forms export: .csv, or the .zip from 'Download responses'")
    parser.add_argument(
        "--out", type=Path, default=Path(__file__).resolve().parent.parent / "Evidence" / "playtest",
        help="evidence folder (default: Evidence/playtest in this repository)",
    )
    parser.add_argument(
        "--fix-player", action="append", default=[], metavar="TYPED=ASSIGNED",
        help="a player typed a wrong number in the form, e.g. 1810=4; saved in coding.csv, so it is needed once",
    )
    parser.add_argument(
        "--fix-row", action="append", default=[], metavar="ROW=ASSIGNED",
        help="two players typed the same number; give the export row (row 1 is the header) and the assigned number, e.g. 9=8",
    )
    arguments = parser.parse_args()
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(errors="replace")
    try:
        run(arguments.export, arguments.out, arguments.fix_player, arguments.fix_row)
    except ReportError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    except OSError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
