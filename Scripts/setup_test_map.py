# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Phase 3: input assets, GM_TD, PC_TD and the greybox L_TD_Test (three system cubes, floor, camera).

Run inside the editor (MCP execute_python_code), after Scripts/build_greybox_ui.py:
    exec(open(r"<ProjectDir>/Scripts/setup_test_map.py", encoding="utf-8").read())
Safe to re-run: existing assets are reused, the three cubes and the floor are replaced.
"""

import unreal

ROOT = "/Game/TouchdownMars"
INPUT = f"{ROOT}/Input"
BP = f"{ROOT}/Blueprints"
log = []


def exists(path):
    return unreal.EditorAssetLibrary.does_asset_exist(path)


# Enhanced Input (VibeUE InputService)
for action in ("IA_TD_NextPhase", "IA_TD_Back"):
    if not exists(f"{INPUT}/{action}"):
        unreal.InputService.create_action(action, INPUT, "Digital")
if not exists(f"{INPUT}/IMC_TD_Default"):
    unreal.InputService.create_mapping_context("IMC_TD_Default", INPUT)
    for key in ("SpaceBar", "Enter"):
        unreal.InputService.add_key_mapping(f"{INPUT}/IMC_TD_Default", f"{INPUT}/IA_TD_NextPhase", key)
    unreal.InputService.add_key_mapping(f"{INPUT}/IMC_TD_Default", f"{INPUT}/IA_TD_Back", "Escape")
imc = unreal.load_asset(f"{INPUT}/IMC_TD_Default")
log.append(f"IMC_TD_Default mappings: {len(imc.get_editor_property('default_key_mappings').get_editor_property('mappings')) if hasattr(imc, 'get_editor_property') else '?'}")


def make_blueprint(name, parent_class):
    path = f"{BP}/{name}"
    if not exists(path):
        factory = unreal.BlueprintFactory()
        factory.set_editor_property("parent_class", parent_class)
        unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, BP, unreal.Blueprint, factory)
    bp = unreal.load_asset(path)
    return bp, unreal.get_default_object(bp.generated_class())


pc_bp, pc = make_blueprint("PC_TD", unreal.TDPlayerController)
pc.set_editor_property("hud_widget_class", unreal.load_asset(f"{ROOT}/UI/WBP_TD_HUD").generated_class())
pc.set_editor_property("default_mapping_context", imc)
pc.set_editor_property("next_phase_action", unreal.load_asset(f"{INPUT}/IA_TD_NextPhase"))
pc.set_editor_property("back_action", unreal.load_asset(f"{INPUT}/IA_TD_Back"))
unreal.BlueprintEditorLibrary.compile_blueprint(pc_bp)
unreal.EditorAssetLibrary.save_asset(f"{BP}/PC_TD", only_if_is_dirty=False)

gm_bp, gm = make_blueprint("GM_TD", unreal.TDGameMode)
gm.set_editor_property("player_controller_class", pc_bp.generated_class())
unreal.BlueprintEditorLibrary.compile_blueprint(gm_bp)
unreal.EditorAssetLibrary.save_asset(f"{BP}/GM_TD", only_if_is_dirty=False)

# L_TD_Test
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Test")
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property("default_game_mode", gm_bp.generated_class())

by_label = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
for label in ("TD_Module_Air", "TD_Module_Greenhouse", "TD_Module_Shelter", "TD_Floor"):
    if label in by_label:
        actors.destroy_actor(by_label[label])

FLOOR_MI = f"{ROOT}/Materials/MI_TD_Greybox_Floor"
if not exists(FLOOR_MI):
    factory = unreal.MaterialInstanceConstantFactoryNew()
    unreal.AssetToolsHelpers.get_asset_tools().create_asset("MI_TD_Greybox_Floor", f"{ROOT}/Materials", unreal.MaterialInstanceConstant, factory)
floor_mi = unreal.load_asset(FLOOR_MI)
unreal.MaterialEditingLibrary.set_material_instance_parent(floor_mi, unreal.load_asset("/Engine/BasicShapes/BasicShapeMaterial"))
unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(floor_mi, "Color", unreal.LinearColor(0.06, 0.045, 0.035, 1.0))
unreal.MaterialEditingLibrary.update_material_instance(floor_mi)
unreal.EditorAssetLibrary.save_asset(FLOOR_MI, only_if_is_dirty=False)

floor = actors.spawn_actor_from_object(unreal.load_asset("/Engine/BasicShapes/Plane"), unreal.Vector(0, 0, 0))
floor.set_actor_label("TD_Floor")
floor.set_actor_scale3d(unreal.Vector(30, 30, 1))
floor.static_mesh_component.set_material(0, floor_mi)

for index, (system, label) in enumerate(((unreal.TDSystem.AIR, "TD_Module_Air"),
                                         (unreal.TDSystem.GREENHOUSE, "TD_Module_Greenhouse"),
                                         (unreal.TDSystem.SHELTER, "TD_Module_Shelter"))):
    module = actors.spawn_actor_from_class(unreal.TDSystemModule, unreal.Vector(0, -350 + index * 350, 50))
    module.set_actor_label(label)
    module.set_editor_property("system", system)

by_label = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
camera = by_label["TD_TestCamera"]
camera.set_actor_location_and_rotation(unreal.Vector(-1100, 0, 420), unreal.Rotator(pitch=-18, yaw=0, roll=0), False, False)
by_label["TD_Sun"].light_component.set_intensity(2.0)  # dim greybox sun so the system lights read
if "TD_DebugText" in by_label:
    by_label["TD_DebugText"].set_actor_hidden_in_game(True)  # the HUD DebugText block replaces it

les.save_current_level()
log.append("actors: " + ", ".join(sorted(a.get_actor_label() for a in actors.get_all_level_actors() if a.get_actor_label().startswith("TD_"))))
log.append("game mode: " + world.get_world_settings().get_editor_property("default_game_mode").get_name())
print("\n".join(log))
