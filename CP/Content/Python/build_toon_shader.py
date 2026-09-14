import unreal

SHADER_PATH = "/Game/Shader"
MF_NAME = "MF_ToonPlasticLighting"
MAT_NAME = "M_ToonPlastic_Demo"

CUSTOM_CODE = """
float3 N = normalize(WorldNormal);
float3 L = normalize(LightDir);
float3 V = normalize(CameraVec);
float3 H = normalize(L + V);

float NdotL = dot(N, L);
float toonRamp = smoothstep(-RampSoft, RampSoft, NdotL) * 0.7 + 0.3;

float NdotH = saturate(dot(N, H));
float3 spec = pow(NdotH, SpecPower) * 1.5 * SpecularColor;

float fresnel = pow(1.0 - saturate(dot(N, V)), 3.0);
float3 rim = fresnel * RimIntensity;

return BaseColor * toonRamp + spec + rim;
"""

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary


def make_or_load(name, path, factory, klass):
    full = path + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(full):
        unreal.EditorAssetLibrary.delete_asset(full)
    return asset_tools.create_asset(name, path, klass, factory())


def build_material_function():
    mf = make_or_load(MF_NAME, SHADER_PATH, unreal.MaterialFunctionFactoryNew, unreal.MaterialFunction)

    fi_basecolor = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionFunctionInput, -700, -150)
    fi_basecolor.set_editor_property("input_name", "BaseColor")
    fi_basecolor.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_VECTOR3)

    const_white = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionConstant3Vector, -900, -150)
    const_white.set_editor_property("constant", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    mel.connect_material_expressions(const_white, "", fi_basecolor, "")
    fi_basecolor.set_editor_property("use_preview_value_as_default", True)

    normal_ws = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionPixelNormalWS, -700, 0)
    camera_vec = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionCameraVectorWS, -700, 100)

    light_dir = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionVectorParameter, -700, 200)
    light_dir.set_editor_property("parameter_name", "SunLightDirection")
    light_dir.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 1.0, 0.0))
    light_dir.set_editor_property("group", "Toon Lighting")
    light_dir.set_editor_property("sort_priority", 0)
    light_dir.set_editor_property("desc", "메인 라이트 방향 (씬 디렉셔널 라이트 회전과 맞춰주세요)")

    spec_power = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionScalarParameter, -700, 300)
    spec_power.set_editor_property("parameter_name", "SpecPower")
    spec_power.set_editor_property("default_value", 96.0)
    spec_power.set_editor_property("slider_min", 8.0)
    spec_power.set_editor_property("slider_max", 256.0)
    spec_power.set_editor_property("group", "Toon Lighting")
    spec_power.set_editor_property("sort_priority", 1)
    spec_power.set_editor_property("desc", "스펙큘러 하이라이트 크기 (클수록 작고 또렷한 반짝임)")

    rim_intensity = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionScalarParameter, -700, 380)
    rim_intensity.set_editor_property("parameter_name", "RimIntensity")
    rim_intensity.set_editor_property("default_value", 0.4)
    rim_intensity.set_editor_property("slider_min", 0.0)
    rim_intensity.set_editor_property("slider_max", 2.0)
    rim_intensity.set_editor_property("group", "Toon Lighting")
    rim_intensity.set_editor_property("sort_priority", 2)
    rim_intensity.set_editor_property("desc", "테두리(림) 라이트 세기")

    ramp_soft = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionScalarParameter, -700, 460)
    ramp_soft.set_editor_property("parameter_name", "RampSoft")
    ramp_soft.set_editor_property("default_value", 0.05)
    ramp_soft.set_editor_property("slider_min", 0.0)
    ramp_soft.set_editor_property("slider_max", 0.5)
    ramp_soft.set_editor_property("group", "Toon Lighting")
    ramp_soft.set_editor_property("sort_priority", 3)
    ramp_soft.set_editor_property("desc", "명암 경계 부드러움 (작을수록 딱 떨어지는 툰 경계)")

    specular_color = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionVectorParameter, -700, 540)
    specular_color.set_editor_property("parameter_name", "SpecularColor")
    specular_color.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    specular_color.set_editor_property("group", "Toon Lighting")
    specular_color.set_editor_property("sort_priority", 4)
    specular_color.set_editor_property("desc", "스펙큘러 하이라이트 색")

    custom = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionCustom, -300, 100)
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    custom.set_editor_property("description", "ToonPlasticLighting")
    input_names = ["BaseColor", "WorldNormal", "LightDir", "CameraVec", "SpecPower", "RimIntensity", "RampSoft", "SpecularColor"]
    custom_inputs = []
    for n in input_names:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        custom_inputs.append(ci)
    custom.set_editor_property("inputs", custom_inputs)
    custom.set_editor_property("code", CUSTOM_CODE)

    mel.connect_material_expressions(fi_basecolor, "", custom, "BaseColor")
    mel.connect_material_expressions(normal_ws, "", custom, "WorldNormal")
    mel.connect_material_expressions(light_dir, "", custom, "LightDir")
    mel.connect_material_expressions(camera_vec, "", custom, "CameraVec")
    mel.connect_material_expressions(spec_power, "", custom, "SpecPower")
    mel.connect_material_expressions(rim_intensity, "", custom, "RimIntensity")
    mel.connect_material_expressions(ramp_soft, "", custom, "RampSoft")
    mel.connect_material_expressions(specular_color, "", custom, "SpecularColor")

    fo = mel.create_material_expression_in_function(mf, unreal.MaterialExpressionFunctionOutput, 0, 100)
    fo.set_editor_property("output_name", "Result")
    mel.connect_material_expressions(custom, "", fo, "")

    unreal.EditorAssetLibrary.save_loaded_asset(mf)
    unreal.log("Saved material function: " + mf.get_path_name())
    return mf


def build_demo_material(mf):
    mat = make_or_load(MAT_NAME, SHADER_PATH, unreal.MaterialFactoryNew, unreal.Material)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    tex_param = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -400, 0)
    tex_param.set_editor_property("parameter_name", "BaseColor")
    tex_param.set_editor_property("default_value", unreal.LinearColor(0.8, 0.1, 0.1, 1.0))

    func_call = mel.create_material_expression(mat, unreal.MaterialExpressionMaterialFunctionCall, -150, 0)
    func_call.set_editor_property("material_function", mf)

    mel.connect_material_expressions(tex_param, "", func_call, "BaseColor")
    mel.connect_material_property(func_call, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
    unreal.log("Saved demo material: " + mat.get_path_name())


mf = build_material_function()
unreal.log("DONE")
