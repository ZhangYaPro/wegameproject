import unreal as u
from pathlib import Path
import json
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
base='/Game/Ski25D';assets=u.EditorAssetLibrary;tools=u.AssetToolsHelpers.get_asset_tools();lib=u.MaterialEditingLibrary
manifest=json.loads((root/'ContentSource'/'Audio'/'audio_manifest.json').read_text())
for name,data in manifest.items():
    task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'Audio'/(name+'.wav'));task.destination_path=base+'/Audio';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
    tools.import_asset_tasks([task]);sound=u.load_asset(base+'/Audio/'+name)
    assert isinstance(sound,u.SoundWave),name
    sound.set_editor_property('looping',data['loop']);sound.set_sound_asset_compression_type(u.SoundAssetCompressionType.PCM)
    assert abs(sound.get_editor_property('duration')-data['seconds'])<.1,(name,sound.get_editor_property('duration'))
    assets.save_loaded_asset(sound)
for name in ['GondolaJilin','GondolaBeidahu','GondolaJingyuetan','GondolaChagan','GondolaColdNoodles','GondolaRiceCake','GondolaGinseng','GondolaKimchi','Welcome2027','Chapter2_Youth','MenuScene1','MenuScene2','MenuScene3']:
    task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'SkiSample'/(name+'.png'));task.destination_path=base+'/Textures';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True;tools.import_asset_tasks([task])
    if name.startswith('MenuScene'):continue
    path=base+'/Materials/M_'+name
    if assets.does_asset_exist(path):continue
    mat=tools.create_asset('M_'+name,base+'/Materials',u.Material,u.MaterialFactoryNew());mat.set_editor_property('two_sided',True);mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    if name.startswith('Gondola'):mat.set_editor_property('used_with_instanced_static_meshes',True)
    sample=lib.create_material_expression(mat,u.MaterialExpressionTextureSample);sample.texture=u.load_asset(base+'/Textures/'+name)
    lib.connect_material_property(sample,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR);lib.recompile_material(mat);assets.save_loaded_asset(mat)
u.log('SKI_EXPERIENCE_ASSETS_READY audio='+str(len(manifest)))
