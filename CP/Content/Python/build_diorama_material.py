import unreal

SHADER_PATH = "/Game/Shader/Diorama"
MAT_NAME = "M_Diorama_PBR"
MI_NAME = "MI_Diorama_Test"

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def make_or_load(name, path, factory, klass):
    full = path + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    return asset_tools.create_asset(name, path, klass, factory())


def build_material():
    mat = make_or_load(MAT_NAME, SHADER_PATH, unreal.MaterialFactoryNew, unreal.Material)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)

    base_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -500, -150)
    base_color.set_editor_property("parameter_name", "BaseColor")
    base_color.set_editor_property("default_value", unreal.LinearColor(0.8, 0.1, 0.1, 1.0))
    base_color.set_editor_property("group", "Diorama PBR")
    base_color.set_editor_property("sort_priority", 0)

    metallic = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 0)
    metallic.set_editor_property("r", 0.0)

    roughness = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 100)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.35)
    roughness.set_editor_property("slider_min", 0.2)
    roughness.set_editor_property("slider_max", 0.55)
    roughness.set_editor_property("group", "Diorama PBR")
    roughness.set_editor_property("sort_priority", 1)
    roughness.set_editor_property("desc", "재질 거칠기 (낮을수록 플라스틱처럼 반짝임)")

    specular = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 200)
    specular.set_editor_property("parameter_name", "Specular")
    specular.set_editor_property("default_value", 0.8)
    specular.set_editor_property("slider_min", 0.5)
    specular.set_editor_property("slider_max", 1.0)
    specular.set_editor_property("group", "Diorama PBR")
    specular.set_editor_property("sort_priority", 2)
    specular.set_editor_property("desc", "스펙큘러 반사 강도 (기본 0.5보다 강조)")

    mel.connect_material_property(base_color, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(metallic, "", unreal.MaterialProperty.MP_METALLIC)
    mel.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(specular, "", unreal.MaterialProperty.MP_SPECULAR)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("Saved diorama material: " + mat.get_path_name())
    return mat


def build_instance(mat):
    full = SHADER_PATH + "/" + MI_NAME
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    factory = unreal.MaterialInstanceConstantFactoryNew()
    mi = asset_tools.create_asset(MI_NAME, SHADER_PATH, unreal.MaterialInstanceConstant, factory)
    mel.set_material_instance_parent(mi, mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    unreal.log("Saved diorama MI: " + mi.get_path_name())


mat = build_material()
build_instance(mat)
unreal.log("DONE")
