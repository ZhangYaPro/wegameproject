"""Save revised cabins and add two safely elevated welcome boards per chapter."""
import unreal as u
from pathlib import Path
import json
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
base='/Game/Ski25D';actors=u.get_editor_subsystem(u.EditorActorSubsystem);levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
reports=[]
def box(label,location,scale,material,mesh='Cube',rotation=u.Rotator()):
    actor=next((a for a in actors.get_all_level_actors() if a.get_actor_label()==label),None)
    if actor is None:actor=actors.spawn_actor_from_class(u.StaticMeshActor,u.Vector(*location),rotation)
    actor.set_actor_label(label);actor.set_actor_location(u.Vector(*location),False,False);actor.set_actor_rotation(rotation,False);actor.set_actor_scale3d(u.Vector(*scale))
    actor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/'+mesh))
    actor.static_mesh_component.set_material(0,u.load_asset(base+'/Materials/M_'+material));actor.static_mesh_component.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    return actor
for index,name in enumerate(['SkiRimeMap','SkiBeidahuMap','SkiChangbaiMap']):
    assert levels.load_level(base+'/Maps/'+name)
    trail=next(a for a in actors.get_all_level_actors() if isinstance(a,u.SkiSampleTrail));trail.rebuild_course()
    height=u.SkiSampleTrail.height_at;finish=u.SkiSampleTrail.get_finish_x();clearances=[]
    for i,x in enumerate([2500,finish-8100]):
        half=500;bottom=max(height(x+dx) for dx in range(-half,half+1,10))+85;z=bottom+250;label='Welcome2027_'+str(i)
        # Keep the sign behind the skier, but ahead of roadside lamps and branches.
        box(label+'_Back',(x,-140,z),(10.15,.22,5.15),'Midnight')
        box(label,(x,-122,z),(10,5,1),'Welcome2027','Plane',u.Rotator(roll=90))
        box(label+'_Cap',(x,-140,z+264),(10.4,.5,.12),'Snow')
        for j,dx in enumerate([-395,395]):
            foot=height(x+dx)-10;top=bottom+30;box(label+'_Post'+str(j),(x+dx,-142,(foot+top)/2),(.22,.24,(top-foot)/100),'Cedar')
        gap=min(bottom-height(x+dx) for dx in range(-half,half+1,10));assert gap>=84
        clearances.append(gap)
    assert levels.save_current_level()
    record=dict(map=name,welcome_boards=2,min_clearance_cm=min(clearances))
    if index==1:
        components=trail.get_components_by_class(u.HierarchicalInstancedStaticMeshComponent)
        counts={c.get_name():c.get_instance_count() for c in components if c.get_name() in [f'Scenery{i:02d}' for i in range(14,22)]}
        assert len(counts)==8 and all(v>0 for v in counts.values()),counts
        record['cabin_panel_variants']=counts
    reports.append(record)
(root/'Saved'/'SkiQA'/'welcome_boards.json').write_text(json.dumps(reports,indent=2))
u.log('SKI_EXPERIENCE_MAPS_READY')
