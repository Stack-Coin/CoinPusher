import unreal

MAP_PATH = "/Game/Maps/UTJ/ShaderTest"

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

unreal.EditorLevelLibrary.load_level(MAP_PATH)

actors = actor_subsystem.get_all_level_actors()

ppv = None
directional_light = None
sky_light = None
for a in actors:
    if isinstance(a, unreal.PostProcessVolume) and ppv is None:
        ppv = a
    if isinstance(a, unreal.DirectionalLight) and directional_light is None:
        directional_light = a
    if isinstance(a, unreal.SkyLight) and sky_light is None:
        sky_light = a

if ppv is None:
    ppv = actor_subsystem.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    unreal.log("Spawned new PostProcessVolume")
else:
    unreal.log("Reusing existing PostProcessVolume")

ppv.set_editor_property("unbound", True)

settings = ppv.get_editor_property("settings")
settings.set_editor_property("override_depth_of_field_fstop", True)
settings.set_editor_property("depth_of_field_fstop", 1.4)
settings.set_editor_property("override_depth_of_field_focal_distance", True)
settings.set_editor_property("depth_of_field_focal_distance", 900.0)
settings.set_editor_property("override_bloom_intensity", True)
settings.set_editor_property("bloom_intensity", 0.3)
settings.set_editor_property("override_vignette_intensity", True)
settings.set_editor_property("vignette_intensity", 0.15)
settings.set_editor_property("override_color_saturation", True)
settings.set_editor_property("color_saturation", unreal.Vector4(1.15, 1.15, 1.15, 1.0))
ppv.set_editor_property("settings", settings)

if directional_light is None:
    unreal.log("WARNING: no DirectionalLight found in level")
else:
    light_comp = directional_light.get_component_by_class(unreal.DirectionalLightComponent)
    light_comp.set_editor_property("intensity", 8.0)
    light_comp.set_editor_property("light_source_angle", 3.0)
    unreal.log("Adjusted DirectionalLight intensity/source_angle")

if sky_light is None:
    sky_light = actor_subsystem.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    sky_comp = sky_light.get_component_by_class(unreal.SkyLightComponent)
    sky_comp.set_editor_property("intensity", 1.0)
    sky_comp.recapture_sky()
    unreal.log("Spawned new SkyLight")
else:
    unreal.log("Reusing existing SkyLight")

unreal.EditorLevelLibrary.save_current_level()
unreal.log("DONE")
