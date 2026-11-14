# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Create or re-import every DT_TD_* DataTable from Data/Source/*.csv (the CSV is the source of truth).

Run inside the Unreal Editor (Python), for example through the MCP tool execute_python_code:
    exec(open(r"<ProjectDir>/Scripts/import_data.py", encoding="utf-8").read())
Safe to run any number of times. Prints one line per table and a final IMPORT OK / IMPORT FAILED line.
Also checks the constants rule: a row with bIsDesignerValue=False needs a SourceId that exists in sources.csv
with a URL.
"""

import csv
import os

import unreal

DATA_DIR = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Data", "Source")
ASSET_DIR = "/Game/TouchdownMars/Data"

# (csv file, DataTable asset name, row struct)
TABLES = [
    ("phases.csv", "DT_TD_Phases", "/Script/TouchdownMars.TDPhaseRow"),
    ("events.csv", "DT_TD_Events", "/Script/TouchdownMars.TDEventRow"),
    ("ageband.csv", "DT_TD_AgeBands", "/Script/TouchdownMars.TDAgeBandRow"),
    ("constants.csv", "DT_TD_Constants", "/Script/TouchdownMars.TDConstantRow"),
    ("sources.csv", "DT_TD_Sources", "/Script/TouchdownMars.TDSourceRow"),
    ("text.csv", "DT_TD_Text", "/Script/TouchdownMars.TDTextRow"),
    ("donki_archive.csv", "DT_TD_DonkiArchive", "/Script/TouchdownMars.TDDonkiRow"),
    ("outcome_shots.csv", "DT_OutcomeShots", "/Script/TouchdownMars.OutcomeShot"),
    ("mission_options.csv", "DT_TD_MissionOptions", "/Script/TouchdownMars.TDDesignOptionRow"),
    ("objectives.csv", "DT_TD_Objectives", "/Script/TouchdownMars.TDObjectiveRow"),
    ("mars_weather.csv", "DT_TD_MarsWeather", "/Script/TouchdownMars.TDWeatherRow"),
    ("sites.csv", "DT_TD_Sites", "/Script/TouchdownMars.TDSiteRow"),
    ("tasks.csv", "DT_TD_Tasks", "/Script/TouchdownMars.TDTaskRow"),
]


def report(message):
    """print() reaches the MCP tool output; unreal.log reaches the log file in commandlet runs."""
    print(message)
    unreal.log(message)


def read_csv_rows(path):
    with open(path, encoding="utf-8", newline="") as f:
        return list(csv.DictReader(f))


def get_or_create_table(asset_name, struct_path):
    asset_path = f"{ASSET_DIR}/{asset_name}"
    row_struct = unreal.load_object(None, struct_path)
    if row_struct is None:
        raise RuntimeError(f"Row struct {struct_path} not found; build the C++ module first.")
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        table = unreal.load_asset(asset_path)
        if table.get_editor_property("row_struct") != row_struct:
            raise RuntimeError(f"{asset_path} uses another row struct; delete it and run again.")
        return table, False
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, ASSET_DIR, unreal.DataTable, factory)
    if table is None:
        raise RuntimeError(f"Could not create {asset_path}.")
    return table, True


def check_constants():
    sources = {r["Name"]: r for r in read_csv_rows(os.path.join(DATA_DIR, "sources.csv"))}
    problems = []
    for row in read_csv_rows(os.path.join(DATA_DIR, "constants.csv")):
        if row["bIsDesignerValue"].strip().lower() == "true":
            continue
        source = sources.get(row["SourceId"])
        if source is None or not source["URL"].strip():
            problems.append(f"{row['Name']}: SourceId '{row['SourceId']}' missing or without URL")
    return problems


def main():
    ok = True
    for csv_name, asset_name, struct_path in TABLES:
        csv_path = os.path.join(DATA_DIR, csv_name)
        expected = len(read_csv_rows(csv_path))
        table, created = get_or_create_table(asset_name, struct_path)
        if expected > 0:
            if not unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, csv_path):
                unreal.log_error(f"[import_data] {asset_name}: import from {csv_name} failed")
                ok = False
                continue
        unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
        actual = len(unreal.DataTableFunctionLibrary.get_data_table_row_names(table))
        match = actual == expected
        ok = ok and match
        report(f"[import_data] {asset_name}: {actual}/{expected} rows {'created' if created else 'reimported'} {'OK' if match else 'MISMATCH'}")

    problems = check_constants()
    for p in problems:
        report(f"[import_data] constants rule: {p}")
    ok = ok and not problems
    report("[import_data] IMPORT OK" if ok else "[import_data] IMPORT FAILED")


main()
