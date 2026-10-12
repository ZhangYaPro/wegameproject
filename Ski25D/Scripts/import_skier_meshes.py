import unreal as u
from pathlib import Path
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
tools=u.AssetToolsHelpers.get_asset_tools();lib=u.MaterialEditingLibrary;assets=u.EditorAssetLibrary;base='/Game/Ski25D'
for obj in (root/'ContentSource'/'Skier').glob('*.obj'):
    task=u.AssetImportTask();task.filename=str(obj);task.destination_path=base+'/Skier';task.destination_name='SM_'+obj.stem;task.automated=True;task.save=True;task.replace_existing=True
    opts=u.FbxImportUI();opts.import_mesh=True;opts.import_materials=False;opts.import_textures=False;opts.import_as_skeletal=False
    opts.mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH
    opts.static_mesh_import_data.set_editor_property('combine_meshes',True)
    opts.static_mesh_import_data.set_editor_property('convert_scene',False)
    task.options=opts;tools.import_asset_tasks([task])
    if not assets.does_asset_exist(base+'/Skier/SM_'+obj.stem):raise RuntimeError('Missing '+obj.stem)
for name,col,rough,metal in [('RaceSkin',(.65,.38,.24),.8,0),('RaceRed',(.65,.022,.035),.68,0),('RaceWhite',(.85,.90,.91),.62,0),('RaceNavy',(.025,.042,.062),.72,0),('RaceVisor',(.09,.30,.40),.19,.72),('FlagRed',(.80,.018,.027),.65,0),('FlagGold',(1,.72,.015),.6,0),('GlacierIce',(.17,.51,.66),.27,.12)]:
    path=base+'/Materials/M_'+name
    if assets.does_asset_exist(path):continue
    m=tools.create_asset('M_'+name,base+'/Materials',u.Material,u.MaterialFactoryNew());m.set_editor_property('two_sided',name.startswith('Flag'));m.set_editor_property('used_with_instanced_static_meshes',True)
    c=lib.create_material_expression(m,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(*col,1));lib.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
    for value,prop in [(rough,u.MaterialProperty.MP_ROUGHNESS),(metal,u.MaterialProperty.MP_METALLIC)]:
        n=lib.create_material_expression(m,u.MaterialExpressionConstant);n.set_editor_property('r',value);lib.connect_material_property(n,'',prop)
    lib.recompile_material(m);assets.save_loaded_asset(m)
u.log('SKI_REMODEL_ASSETS_READY')
