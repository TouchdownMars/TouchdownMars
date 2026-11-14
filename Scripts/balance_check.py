# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Play 1,000 seeds per band and policy with the real DataTables and print the loss rate by phase.

Run inside the editor (MCP execute_python_code):
    exec(open(r"<ProjectDir>/Scripts/balance_check.py", encoding="utf-8").read())
or headless:
    UnrealEditor-Cmd.exe TouchdownMars.uproject -run=pythonscript -script="<ProjectDir>/Scripts/balance_check.py"
Targets (technical_design.md §4, designer values): phase 1 loss 0 %; random-play loss between 20 % and 50 %.
The model runs in C++ (UTDMissionRules::SimulateFromTables), so this script only reads tables and prints.
"""

import unreal

SEEDS = 1000
FIRST_SEED = 1000
DATA = "/Game/TouchdownMars/Data/"
PHASES = ["Arrival", "FirstWalk", "Exploration", "Resupply", "HardWeek", "Return"]


def report(message):
    print(message)
    unreal.log(message)


def load(name):
    table = unreal.load_asset(DATA + name)
    if table is None:
        raise RuntimeError(f"{name} missing; run Scripts/import_data.py")
    return table


def main():
    phases, bands, constants, events = (load(n) for n in ("DT_TD_Phases", "DT_TD_AgeBands", "DT_TD_Constants", "DT_TD_Events"))
    card_count = len(unreal.DataTableFunctionLibrary.get_data_table_row_names(events))
    report(f"[balance] {SEEDS} seeds per run, {card_count} event cards in DT_TD_Events")
    for band in (unreal.TDAgeBand.COMMANDER, unreal.TDAgeBand.CADET):
        for policy in (unreal.TDAllocationPolicy.RANDOM, unreal.TDAllocationPolicy.GREEDY):
            r = unreal.TDMissionRules.simulate_from_tables(phases, bands, constants, events, band, policy, FIRST_SEED, SEEDS)
            by_phase = ", ".join(f"{PHASES[i]} {n / max(r.missions, 1):.1%}" for i, n in enumerate(r.losses_by_phase) if n)
            report(
                f"[balance] {band.name:<9} {policy.name:<6} loss {r.losses / max(r.missions, 1):6.1%}"
                f" | full {r.full_missions / max(r.missions, 1):6.1%} minimum {r.minimum_missions / max(r.missions, 1):6.1%}"
                f" | with events {r.missions_with_events}/{r.missions} | by phase: {by_phase or 'none'}"
            )
    report("[balance] DONE")


main()
