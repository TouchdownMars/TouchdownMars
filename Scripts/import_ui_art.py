# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Import the design-room art and the UI font into /Game/TouchdownMars/UI.

Art: D:/NASA/_kaynaklar/nasa_ui/T_TD_Opt_<OptionId>.png, T_TD_Obj_<ObjectiveId>.png and T_TD_ColdOpen_Ares0.png: Google Gemini
illustrations in one locked style, checked for logos, flags, text and faces (sources.csv GeminiIllustrations).
The earlier NASA photos are kept in D:/NASA/_kaynaklar/nasa_ui_photos_backup.
Fonts: Barlow (SIL OFL 1.1) is copied as raw TTF files to Content/TouchdownMars/UI/Fonts and loaded at runtime.
Headless: UnrealEditor-Cmd.exe TouchdownMars.uproject -run=pythonscript -script=<abs>/Scripts/import_ui_art.py -unattended
"""

import glob
import os

import unreal

ART_SRC = r"D:/NASA/_kaynaklar/nasa_ui"
FONT_SRC = r"D:/NASA/_kaynaklar/fonts"
ART_DST = "/Game/TouchdownMars/UI/Art"
FONT_DST = "/Game/TouchdownMars/UI/Fonts"
tools = unreal.AssetToolsHelpers.get_asset_tools()


def import_files(files, dest):
    tasks = []
    for path in files:
        task = unreal.AssetImportTask()
        task.filename = path
        task.destination_path = dest
        task.destination_name = os.path.splitext(os.path.basename(path))[0]
        task.replace_existing = True
        task.automated = True
        task.save = True
        tasks.append(task)
    tools.import_asset_tasks(tasks)
    return [t.get_editor_property("imported_object_paths") for t in tasks]


art = sorted(glob.glob(os.path.join(ART_SRC, "*.png")))
import_files(art, ART_DST)
for path in art:
    name = os.path.splitext(os.path.basename(path))[0]
    tex = unreal.load_asset(f"{ART_DST}/{name}")
    if tex:
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        tex.set_editor_property("srgb", True)
        unreal.EditorAssetLibrary.save_loaded_asset(tex, only_if_is_dirty=False)

# Fonts are not imported: the game loads Content/TouchdownMars/UI/Fonts/TDFont_*.ttf directly (staged as non-UFS files,
# DefaultGame.ini DirectoriesToAlwaysStageAsNonUFS); importing TTFs needs the full editor UI.
fonts = sorted(glob.glob(os.path.join(FONT_SRC, "TDFont_*.ttf")))

msg = f"[import_ui_art] art {len(art)}, fonts {len(fonts)}"
print(msg)
unreal.log(msg)
