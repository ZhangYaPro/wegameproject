import unreal as u
from pathlib import Path
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'SkiSample'/'MenuScene2.png')
task.destination_path='/Game/Ski25D/Textures';task.destination_name='MenuScene2';task.automated=True;task.save=True;task.replace_existing=True
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
if not levels.load_level('/Game/Ski25D/Maps/SkiChangbaiMap'):raise RuntimeError('load')
for actor in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if isinstance(actor,u.SkiSampleTrail):actor.rebuild_course()
if not levels.save_current_level():raise RuntimeError('save')
u.log('SKI_REVISION_FINAL_ASSETS_READY')
