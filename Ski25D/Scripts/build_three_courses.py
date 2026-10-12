"""Create three independent maps from the preserved sample lighting setup."""
import unreal as u
from pathlib import Path
import json,math
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
base='/Game/Ski25D';assets=u.EditorAssetLibrary;lib=u.MaterialEditingLibrary
level=u.get_editor_subsystem(u.LevelEditorSubsystem);actors=u.get_editor_subsystem(u.EditorActorSubsystem)
tools=u.AssetToolsHelpers.get_asset_tools()
def sign_material(name):
    task=u.AssetImportTask();task.filename=str(root/'ContentSource'/'SkiSample'/(name+'.png'))
    task.destination_path=base+'/Textures';task.destination_name=name;task.automated=True;task.save=True;task.replace_existing=True
    tools.import_asset_tasks([task])
    path=base+'/Materials/M_'+name
    mat=u.load_asset(path) if assets.does_asset_exist(path) else tools.create_asset('M_'+name,base+'/Materials',u.Material,u.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat);mat.set_editor_property('two_sided',True);mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    sample=lib.create_material_expression(mat,u.MaterialExpressionTextureSample);sample.texture=u.load_asset(base+'/Textures/'+name)
    mult=lib.create_material_expression(mat,u.MaterialExpressionMultiply)
    strength=lib.create_material_expression(mat,u.MaterialExpressionConstant);strength.set_editor_property('r',.75)
    lib.connect_material_expressions(sample,'RGB',mult,'A');lib.connect_material_expressions(strength,'',mult,'B');lib.connect_material_property(mult,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat);assets.save_loaded_asset(mat);return mat
names=['Chapter1_Entry','Chapter1_End','Chapter2_Entry','Chapter2_Lift','Chapter2_Youth','Chapter2_End','Chapter3_Entry','Chapter3_Ravine','Chapter3_Lodge','Chapter3_End']
mats={name:sign_material(name) for name in names}
def box(label,pos,scale,mat,shape='Cube',rot=None):
    a=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*pos),rot or u.Rotator())
    a.set_actor_label(label);a.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/'+shape))
    a.static_mesh_component.set_material(0,mat if not isinstance(mat,str) else u.load_asset(base+'/Materials/M_'+mat))
    a.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION);a.set_actor_scale3d(u.Vector(*scale));return a
reports=[]
for index,map_name in enumerate(['SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap']):
    if not level.load_level(base+'/Maps/SkiChallengeMap'):raise RuntimeError('Cannot load preserved template')
    # Save a distinct asset BEFORE editing any actor in the preserved map.
    if not u.EditorLoadingAndSavingUtils.save_map(level.get_current_level().get_outer(),base+'/Maps/'+map_name):raise RuntimeError('Cannot save independent chapter')
    # SaveMap writes a copy but does not switch the editor's active world.
    if not level.load_level(base+'/Maps/'+map_name):raise RuntimeError('Cannot activate new chapter copy')
    assert level.get_current_level().get_outer().get_name()==map_name
    all_actors=list(actors.get_all_level_actors());trail=next(a for a in all_actors if isinstance(a,u.SkiSampleTrail))
    trail.set_editor_property('course_index',index);trail.rebuild_course();trail.set_actor_label(['01 雾凇江岸','02 北大湖挑战','03 长白林海'][index])
    height=u.SkiSampleTrail.height_at;finish=u.SkiSampleTrail.get_finish_x()
    for actor in all_actors:
        label=actor.get_actor_label()
        if isinstance(actor,u.PlayerStart):actor.set_actor_location(u.Vector(200,0,height(200)+91),False,False)
        if label.startswith('Chapter') or (label.startswith('Sign_') and (index>0 or label.startswith('Sign_Start') or label.startswith('Sign_Finish'))):actors.destroy_actor(actor)
    signs=([(900,'Chapter1_Entry'),(finish-1600,'Chapter1_End')] if index==0 else
           [(900,'Chapter2_Entry'),(35800,'Chapter2_Lift'),(87500,'Chapter2_Youth'),(finish-1700,'Chapter2_End')] if index==1 else
           [(900,'Chapter3_Entry'),(36200,'Chapter3_Ravine'),(113500,'Chapter3_Lodge'),(finish-1700,'Chapter3_End')])
    clearances=[]
    for x,name in signs:
        bottom=max(height(x+dx) for dx in range(-350,351,10))+90;centre=bottom+167.5
        box(name+'_Back',(x,-255,centre),(6.6,.23,3.35),'Midnight')
        box(name,(x,-240,centre),(6.4,3.2,1),mats[name],'Plane',u.Rotator(roll=90))
        box(name+'_Cap',(x,-255,centre+172),(6.9,.5,.12),'Snow')
        for dx in [-245,245]:
            foot=height(x+dx)-10;top=bottom+30
            box(name+'_Post',(x+dx,-260,(foot+top)/2),(.17,.22,(top-foot)/100),'Cedar')
        clearance=min(bottom-height(x+dx) for dx in range(-330,331,10));assert clearance>=85
        clearances.append(round(clearance,2))
    if not level.save_current_level():raise RuntimeError('Cannot save chapter actors')
    samples=[height(x) for x in range(200,int(finish)+1,50)]
    max_slope=max(abs(math.degrees(math.atan((b-a)/50))) for a,b in zip(samples,samples[1:]))
    assert max_slope<55
    reports.append(dict(chapter=index+1,map=map_name,length_m=(finish-200)/100,max_slope_deg=round(max_slope,2),height_drop_m=round((height(200)-height(finish))/100,2),sign_clearance_cm=clearances,height_samples=samples[::100]))
    u.log('SKI_INDEPENDENT_MAP_READY '+map_name)
out=root/'Saved'/'SkiQA'/'three_courses.json';out.parent.mkdir(parents=True,exist_ok=True)
out.write_text(json.dumps(dict(passed=True,courses=reports),ensure_ascii=False,indent=2),encoding='utf-8')
u.log('SKI_THREE_COURSES_READY')
