"""Re-anchor existing tourism signs on slopes without rebuilding the scenery."""
import json
from pathlib import Path
import unreal as u

level = u.get_editor_subsystem(u.LevelEditorSubsystem)
if not level.load_level('/Game/Ski25D/Maps/SkiChallengeMap'):
    raise RuntimeError('Cannot load challenge map')
actors = list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
height = u.SkiSampleTrail.height_at
names = ['Sign_Start', 'Sign_Rime', 'Sign_Lodge', 'Sign_Changbai',
         'Sign_Chagan', 'Sign_Yanbian', 'Sign_Tram', 'Sign_Festival', 'Sign_Finish']
groups = []
for name in names:
    face = [a for a in actors if a.get_actor_label() == name]
    back = [a for a in actors if a.get_actor_label() == name+'_Back']
    cap = [a for a in actors if a.get_actor_label() == name+'_Cap']
    posts = [a for a in actors if a.get_actor_label().startswith(name+'_Post')]
    if (len(face), len(back), len(cap), len(posts)) != (1, 1, 1, 2):
        raise RuntimeError('Unexpected sign group: '+name)
    groups.append((name, face[0], back[0], cap[0], posts))

def move_z(actor, z):
    p = actor.get_actor_location()
    p.z = z
    actor.set_actor_location(p, False, False)

report = []
for name, face, back, cap, posts in groups:
    x = face.get_actor_location().x
    bottom = max(height(x+dx) for dx in range(-350, 351, 10))+85
    centre = bottom+167.5
    old_z = face.get_actor_location().z
    move_z(face, centre)
    move_z(back, centre)
    move_z(cap, centre+172)
    bases = []
    for post in posts:
        foot = height(post.get_actor_location().x)-10
        top = bottom+30
        scale = post.get_actor_scale3d()
        scale.z = (top-foot)/100
        post.set_actor_scale3d(scale)
        move_z(post, (foot+top)/2)
        origin, extent = post.get_actor_bounds(False)
        bases.append(origin.z-extent.z-height(origin.x))
    origin, extent = back.get_actor_bounds(False)
    clearance = min(origin.z-extent.z-height(origin.x-extent.x+i*2*extent.x/200)
                    for i in range(201))
    if clearance < 80 or any(abs(v+10) > .1 for v in bases):
        raise RuntimeError('Sign clearance check failed: '+name)
    report.append(dict(name=name, x_cm=x, raised_cm=round(centre-old_z, 2),
                       min_board_clearance_cm=round(clearance, 2),
                       post_base_offsets_cm=[round(v, 2) for v in bases]))
if not level.save_current_level():
    raise RuntimeError('Cannot save adjusted signs')
target = Path(u.Paths.project_saved_dir())/'SkiQA'/'sign_clearance.json'
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text(json.dumps(dict(passed=True, signs=report), indent=2), encoding='utf-8')
u.log('SKI_SIGNS_FIXED: all 9 boards clear snow, all 18 posts anchored')
