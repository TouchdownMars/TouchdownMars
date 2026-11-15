# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Graphics pass for L_TD_Outpost on top of setup_environment.py: volumetric dust in the air, light shafts from the
sun through it, contact shadows that seat crew, rocks and props on the ground, local exposure so shaded module sides
keep detail, a little motion blur. Look values (no NASA number); the forward-scattering dust (g about 0.7) follows
Tomasko et al. 1999 as cited in research_notes/Mars VFX/vfx_research.md. Re-runnable; headless or in the editor:
    UnrealEditor-Cmd <uproject> -run=pythonscript -script=<ProjectDir>/Scripts/setup_graphics.py
"""

import unreal

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level("/Game/TouchdownMars/Maps/L_TD_Outpost")
log = []


def find(label):
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError(f"{label} not found")


def apply(obj, pairs, tag):
    for prop, value in pairs:
        try:
            obj.set_editor_property(prop, value)
            log.append(f"{tag}.{prop} = {value}")
        except Exception as error:  # names differ between engine versions: skip what does not exist
            log.append(f"skip {tag}.{prop}: {error}")


# Volumetric fog: the dust in the air catches sunlight; forward scattering makes it glow toward the sun.
fog = find("TD_DustHaze").get_component_by_class(unreal.ExponentialHeightFogComponent)
apply(fog, (
    ("enable_volumetric_fog", True),
    ("volumetric_fog_scattering_distribution", 0.7),
    ("volumetric_fog_albedo", unreal.Color(r=235, g=190, b=150, a=255)),
    ("volumetric_fog_extinction_scale", 1.2),
    ("volumetric_fog_start_distance", 0.0),
    ("volumetric_fog_distance", 20000.0),
    ("volumetric_fog_static_lighting_scattering_intensity", 1.0),
), "fog")

# Sun: shafts through the volumetric dust, contact shadows for small objects and feet.
sun = find("TD_Sun").get_component_by_class(unreal.DirectionalLightComponent)
apply(sun, (
    ("volumetric_scattering_intensity", 1.6),
    ("cast_volumetric_shadow", True),
    ("contact_shadow_length", 0.04),
    ("enable_light_shaft_bloom", True),
    ("bloom_scale", 0.08),
    ("bloom_threshold", 4.0),
), "sun")

# Sky light follows the moving sun and the storm (ambient light from the real sky).
sky_light = find("TD_SkyLight").get_component_by_class(unreal.SkyLightComponent)
apply(sky_light, (("real_time_capture", True),), "skylight")

# Post-process: keep shaded module sides readable, slight contrast, gentle motion blur, AO for contact.
ppv = find("TD_PostProcess")
s = ppv.get_editor_property("settings")
apply(s, (
    ("override_local_exposure_shadow_contrast_scale", True), ("local_exposure_shadow_contrast_scale", 0.8),
    ("override_local_exposure_highlight_contrast_scale", True), ("local_exposure_highlight_contrast_scale", 0.85),
    ("override_color_contrast", True), ("color_contrast", unreal.Vector4(1.04, 1.04, 1.04, 1.04)),
    ("override_motion_blur_amount", True), ("motion_blur_amount", 0.25),
    ("override_motion_blur_max", True), ("motion_blur_max", 2.0),
    ("override_ambient_occlusion_intensity", True), ("ambient_occlusion_intensity", 0.6),
    ("override_ambient_occlusion_radius", True), ("ambient_occlusion_radius", 120.0),
    ("override_scene_fringe_intensity", True), ("scene_fringe_intensity", 0.15),
), "post")
ppv.set_editor_property("settings", s)

les.save_current_level()
print("\n".join(log))
print("setup_graphics: done")
