"""Repair existing maps in place, preserving level edits and gameplay actors."""
import unreal as u
from pathlib import Path
import json
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
base='/Game/Ski25D';assets=u.EditorAssetLibrary;lib=u.MaterialEditingLibrary
tools=u.AssetToolsHelpers.get_asset_tools();actors=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
for name in ['GondolaJilin','GondolaBeidahu','MenuScene1','MenuScene2','MenuScene3','Chapter2_End','Chapter3_Entry','Chapter3_Ravine','Chapter3_Lodge','Chapter3_End']:
    task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'SkiSample'/(name+'.png'));task.destination_path=base+'/Textures';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;tools.import_asset_tasks([task])
    if name.startswith('Menu'):continue
    path=base+'/Materials/M_'+name
    existing=assets.does_asset_exist(path)
    mat=u.load_asset(path) if existing else tools.create_asset('M_'+name,base+'/Materials',u.Material,u.MaterialFactoryNew())
    mat.set_editor_property('two_sided',True);mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    if name.startswith('Gondola'):mat.set_editor_property('used_with_instanced_static_meshes',True)
    if existing:
        lib.recompile_material(mat);assets.save_loaded_asset(mat);continue
    sample=lib.create_material_expression(mat,u.MaterialExpressionTextureSample);sample.texture=u.load_asset(base+'/Textures/'+name)
    lib.connect_material_property(sample,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR);lib.recompile_material(mat);assets.save_loaded_asset(mat)
reports=[]
for index,name in enumerate(['SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap']):
    if not levels.load_level(base+'/Maps/'+name):raise RuntimeError(name)
    fixed=[]
    for a in actors.get_all_level_actors():
        if not isinstance(a,u.StaticMeshActor):continue
        mesh=a.static_mesh_component.static_mesh;label=a.get_actor_label()
        if mesh and mesh.get_name()=='Plane' and (label.startswith('Sign_') or label.startswith('Chapter')):
            a.set_actor_rotation(u.Rotator(roll=90),False)
            a.set_actor_scale3d(u.Vector(6.4,3.2,1))
            fixed.append(label)
    if not levels.save_current_level():raise RuntimeError('save failed '+name)
    reports.append(dict(map=name,fixed=fixed))
(root/'Saved'/'SkiQA'/'sign_repairs.json').write_text(json.dumps(reports,indent=2),encoding='utf-8')
u.log('SKI_MENU_ASSETS_READY')
