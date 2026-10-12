"""Rebuild the existing trail and re-anchor signs; never extend the route."""
import json
from pathlib import Path
import runpy
import unreal as u

root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
if not level.load_level('/Game/Ski25D/Maps/SkiChallengeMap'):
    raise RuntimeError('Cannot load current map')
actors=list(u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors())
trails=[a for a in actors if isinstance(a,u.SkiSampleTrail)]
if len(trails)!=1:
    raise RuntimeError('Expected exactly one trail')
trails[0].rebuild_course()
for actor in actors:
    if isinstance(actor,u.PlayerStart):
        p=actor.get_actor_location();p.z=u.SkiSampleTrail.height_at(p.x)+91
        actor.set_actor_location(p,False,False)
if not level.save_current_level():
    raise RuntimeError('Cannot save rebuilt snow course')
runpy.run_path(str(root/'Scripts'/'fix_sign_clearance.py'),run_name='__main__')

import math
heights=[(x,u.SkiSampleTrail.height_at(x)) for x in range(200,102001,25)]
segments=[];start=None;max_slope=0
for (ax,az),(bx,bz) in zip(heights,heights[1:]):
    grade=(bz-az)/(bx-ax)
    max_slope=max(max_slope,abs(math.degrees(math.atan(grade))))
    if grade < -.01:
        if start is None:start=ax
    elif start is not None:
        segments.append((start/100,ax/100,(ax-start)/100));start=None
if start is not None:segments.append((start/100,1020,(102000-start)/100))
lengths=[s[2] for s in segments]
assert max(lengths)>120 and min(lengths)<40 and max_slope<50
report=dict(passed=True,route_metres=(102000-200)/100,max_slope_degrees=round(max_slope,2),
            downhill_segments_m=[dict(start=s[0],end=s[1],length=s[2]) for s in segments])
target=root/'Saved'/'SkiQA'/'rolling_course.json';target.parent.mkdir(parents=True,exist_ok=True)
target.write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('SKI_ROLLING_COURSE_READY '+json.dumps(report))
