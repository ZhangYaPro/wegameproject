"""Refresh only the menu images after taking updated in-game scenery screenshots."""
import unreal as u
from pathlib import Path
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
tasks=[]
for i in range(1,4):
    task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'SkiSample'/f'MenuScene{i}.png')
    task.destination_path='/Game/Ski25D/Textures';task.destination_name=f'MenuScene{i}'
    task.automated=True;task.save=True;task.replace_existing=True;tasks.append(task)
u.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
assert all(u.EditorAssetLibrary.does_asset_exist('/Game/Ski25D/Textures/'+t.destination_name) for t in tasks)
u.log('SKI_MENU_PREVIEWS_READY')
