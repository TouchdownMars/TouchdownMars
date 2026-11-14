# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Mars look for L_TD_Outpost: butterscotch daytime sky (weak Rayleigh, blue-absorbing Mie from suspended dust),
warm dust haze and one unbound post-process volume. Values are artistic (no NASA number is shown or implied).
Run in the editor after setup_outpost.py (PIE stopped):
    exec(open(r"<ProjectDir>/Scripts/setup_environment.py", encoding="utf-8").read())
"""

import unreal

ROOT = "/Game/TouchdownMars"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les.load_level(f"{ROOT}/Maps/L_TD_Outpost")


def find(label):
    for actor in actors.get_all_level_actors():
        if actor.get_actor_label() == label:
            return actor
    raise RuntimeError(f"{label} not found")


def lc(r, g, b):
    return unreal.LinearColor(r, g, b, 1.0)


# Sky (research_notes/Mars gokyuzu ve zemin/references.md): dust optical depth about 0.45 at Jezero (Bell et al.
# 2022, Mastcam-Z) over a dust scale height of about 11 km (Lemmon et al. 2004), so Mie extinction is about
# 0.045 per km. The dust absorbs blue: light that has gone a long way (or scattered many times) turns butterscotch,
# while the narrow forward glow around the sun keeps a pale, bluish white. Rayleigh is tiny (thin CO2 air).
sky = find("TD_SkyAtmosphere").get_component_by_class(unreal.SkyAtmosphereComponent)
for prop, value in (
        ("rayleigh_scattering_scale", 0.0008),
        ("rayleigh_scattering", lc(0.20, 0.40, 1.0)),
        ("rayleigh_exponential_distribution", 11.0),
        ("mie_scattering_scale", 0.034),
        ("mie_scattering", lc(0.88, 0.92, 1.0)),
        ("mie_absorption_scale", 0.014),
        ("mie_absorption", lc(0.04, 0.32, 1.0)),
        ("mie_anisotropy", 0.75),
        ("mie_exponential_distribution", 11.0),
        ("ground_albedo", unreal.Color(r=130, g=100, b=80, a=255)),   # broadband albedo about 0.16 (MEDA, OMEGA)
        ("multi_scattering_factor", 1.5)):
    sky.set_editor_property(prop, value)

# Dust haze: warm inscattering, a little denser far away so the horizon softens.
fog = find("TD_DustHaze").get_component_by_class(unreal.ExponentialHeightFogComponent)
fog.set_editor_property("fog_density", 0.0022)
fog.set_editor_property("fog_height_falloff", 0.35)
fog.set_editor_property("fog_inscattering_luminance", lc(0.85, 0.60, 0.40))
fog.set_editor_property("start_distance", 3000.0)

# Sun: slightly warm.
sun = find("TD_Sun").get_component_by_class(unreal.DirectionalLightComponent)
sun.set_editor_property("light_color", unreal.Color(r=255, g=248, b=240, a=255))
sun.set_editor_property("intensity", 6.0)
sun.set_editor_property("light_source_angle", 0.35)   # the sun from Mars: about 0.35 degrees wide (NSSDC, scaled)
sun.set_mobility(unreal.ComponentMobility.MOVABLE)     # UTDSkySubsystem moves it through the sol
find("TD_SkyLight").get_component_by_class(unreal.SkyLightComponent).set_mobility(unreal.ComponentMobility.MOVABLE)

# One unbound post-process volume: stable exposure, gentle warm grade, soft vignette, light bloom.
for actor in actors.get_all_level_actors():
    if actor.get_actor_label() == "TD_PostProcess":
        actors.destroy_actor(actor)
ppv = actors.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
ppv.set_actor_label("TD_PostProcess")
ppv.set_editor_property("unbound", True)
s = ppv.get_editor_property("settings")
for prop, value in (
        ("override_auto_exposure_min_brightness", True), ("auto_exposure_min_brightness", 0.6),
        ("override_auto_exposure_max_brightness", True), ("auto_exposure_max_brightness", 1.6),
        ("override_auto_exposure_bias", True), ("auto_exposure_bias", 0.6),
        ("override_vignette_intensity", True), ("vignette_intensity", 0.35),
        ("override_bloom_intensity", True), ("bloom_intensity", 0.35),
        ("override_color_saturation", True), ("color_saturation", unreal.Vector4(1.05, 1.0, 0.95, 1.05)),
        ("override_color_gain", True), ("color_gain", unreal.Vector4(1.03, 1.0, 0.96, 1.0)),
        ("override_film_grain_intensity", True), ("film_grain_intensity", 0.0)):
    try:
        s.set_editor_property(prop, value)
    except Exception as error:  # property names differ between engine versions; skip what does not exist
        print(f"skip {prop}: {error}")
ppv.set_editor_property("settings", s)

les.save_current_level()
print("environment ok")
