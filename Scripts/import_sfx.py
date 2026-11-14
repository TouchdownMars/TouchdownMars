# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Import Data/Audio/SFX_TD_*.wav as Sound Waves into /Game/TouchdownMars/Audio (run inside the editor).
    exec(open(r"<ProjectDir>/Scripts/import_sfx.py", encoding="utf-8").read())
"""

import glob
import os

import unreal

AUDIO = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "Data", "Audio")
tasks = []
for path in sorted(glob.glob(os.path.join(AUDIO, "SFX_TD_*.wav"))):
    task = unreal.AssetImportTask()
    task.filename = path
    task.destination_path = "/Game/TouchdownMars/Audio"
    task.automated = True
    task.save = True
    task.replace_existing = True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
print("imported:", ", ".join(os.path.splitext(os.path.basename(t.filename))[0] for t in tasks
                             if unreal.EditorAssetLibrary.does_asset_exist("/Game/TouchdownMars/Audio/" + os.path.splitext(os.path.basename(t.filename))[0])))
