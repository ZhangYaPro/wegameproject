import unreal as u
from pathlib import Path
import json
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
base='/Game/Ski25D';assets=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools()
bounds={}
for name in ['FlagCloth','FlagStars','Ski','Jacket']:
    mesh=u.load_asset(base+'/Skier/SM_'+name);box=mesh.get_bounding_box()
    bounds[name]={'min':[box.min.x,box.min.y,box.min.z],'max':[box.max.x,box.max.y,box.max.z]}
(root/'Saved'/'SkiQA'/'skier_mesh_bounds.json').write_text(json.dumps(bounds,indent=2))
# The cloth must stand in X-Z, not lie flat after OBJ axis conversion.
assert abs(bounds['FlagCloth']['max'][1]-bounds['FlagCloth']['min'][1])<.1,bounds
for name in ['MenuScene1','MenuScene2','MenuScene3','Chapter3_Entry']:
    task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'SkiSample'/(name+'.png'));task.destination_path=base+'/Textures';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;tools.import_asset_tasks([task])
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
for name in ['SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap']:
    assert levels.load_level(base+'/Maps/'+name)
    for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor,u.SkiSampleTrail):actor.rebuild_course()
    assert levels.save_current_level()
u.log('SKI_REMODEL_MAPS_READY')
