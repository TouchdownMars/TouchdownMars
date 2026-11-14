# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Give the placed Tripo and NASA models collision. The glTF imports came in without simple collision, and the
model swap (setup_models2.py) turned the placeholder colliders off, so the crew walk grid, the crew and the player
went straight through the new buildings. Complex-as-simple uses the render triangles for queries: exact, and cheap
enough for a handful of static models. Hand tools and the suits stay without collision. Re-runnable.
Run in the editor or headless:
    exec(open(r"<ProjectDir>/Scripts/fix_model_collision.py", encoding="utf-8").read())
"""

import unreal

EAL = unreal.EditorAssetLibrary
ROOTS = ["/Game/TouchdownMars/Tripo", "/Game/TouchdownMars/NASA"]
SKIP = ("ToolScoop", "ToolTablet", "Astronaut", "Hammer", "PistolGripTool", "Wrench", "Z2Suit")
log = []

for root in ROOTS:
    for path in EAL.list_assets(root, recursive=True):
        if any(f"/{name}/" in path for name in SKIP):
            continue
        asset = unreal.load_asset(path)
        if not isinstance(asset, unreal.StaticMesh):
            continue
        body = asset.get_editor_property("body_setup")
        if body is None:
            log.append(f"{path}: no body setup")
            continue
        before = body.get_editor_property("collision_trace_flag")
        body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        asset.set_editor_property("body_setup", body)
        EAL.save_asset(path.split(".")[0], only_if_is_dirty=False)
        log.append(f"{path.split('.')[0]}: {before} -> complex as simple")

print("\n".join(log))
print(f"fix_model_collision: {len(log)} meshes")
