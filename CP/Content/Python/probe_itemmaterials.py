import unreal

mel = unreal.MaterialEditingLibrary
names = ["Black", "Blue", "Green", "Orange", "Purple", "Red", "White", "Yellow"]

for n in names:
    path = "/Game/CoinPusher/ItemMaterial/" + n
    mat = unreal.EditorAssetLibrary.load_asset(path)
    if mat is None:
        unreal.log("MISSING: " + path)
        continue
    unreal.log("=== " + n + " (class=" + mat.get_class().get_name() + ") ===")
    unreal.log("  shading_model: " + str(mat.get_editor_property("shading_model")))
    bc = mat.get_editor_property("base_color")
    expr = bc.get_editor_property("expression")
    unreal.log("  base_color.expression: " + str(expr))
    exprs = mat.get_editor_property("expressions")
    for e in exprs:
        cls = e.get_class().get_name()
        info = cls
        if cls == "MaterialExpressionConstant3Vector":
            info += " constant=" + str(e.get_editor_property("constant"))
        elif cls == "MaterialExpressionVectorParameter":
            info += " param=" + str(e.get_editor_property("parameter_name")) + " default=" + str(e.get_editor_property("default_value"))
        elif cls == "MaterialExpressionConstant":
            info += " r=" + str(e.get_editor_property("r"))
        unreal.log("    expr: " + info)

unreal.log("PROBE_ITEMMAT_DONE")
