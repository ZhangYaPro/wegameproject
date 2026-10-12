"""Refresh only procedural snow geometry and height anchors; preserve scene actors/settings."""
import unreal as u

PATH = '/Game/Ski25D/Maps/SkiChallengeMap'
level = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if not level.load_level(PATH):
    raise RuntimeError('Cannot load challenge map')

points = [(-3000,14000),(0,14000),(1600,13960),(5200,11800),(6200,11700),
          (7100,12050),(9000,10500),(11000,10100),(13500,8550),(14800,8450),
          (15800,8850),(18400,7300),(19900,7450),(21400,7020),(23100,7330),
          (25300,5850),(27100,5800),(28100,6150),(30800,4100),(32900,3850),
          (34100,4150),(37000,2400),(39100,1900),(42000,1850),(47000,1750)]

def old_height(x):
    for (ax, az), (bx, bz) in zip(points, points[1:]):
        if x <= bx:
            t = min(1, max(0, (x-ax)/(bx-ax)))
            return az+(bz-az)*t*t*(3-2*t)
    return points[-1][1]

all_actors = actors.get_all_level_actors()
trails = [a for a in all_actors if isinstance(a, u.SkiSampleTrail)]
if len(trails) != 1:
    raise RuntimeError('Expected exactly one trail; refusing to change scene')
trail = trails[0]
already_refreshed = 'VideoSlopeV1' in [str(t) for t in trail.tags]
transform, label = trail.get_actor_transform(), trail.get_actor_label()
tags = list(trail.tags)
if not already_refreshed:
    tags.append(u.Name('VideoSlopeV1'))
replacement = actors.spawn_actor_from_class(u.SkiSampleTrail, transform.translation, transform.rotation.rotator())
replacement.set_actor_scale3d(transform.scale3d)
replacement.set_actor_label(label)
replacement.set_editor_property('tags', tags)
actors.destroy_actor(trail)
if not already_refreshed:
    for actor in all_actors:
        if actor == trail:
            continue
        actor_label = actor.get_actor_label()
        anchor = next((x for prefix, x in [('Sign_Start',900),('Sign_Rime',10100),
                      ('Sign_Lodge',19500),('Sign_Finish',40400)] if actor_label.startswith(prefix)), None)
        if isinstance(actor, u.PlayerStart):
            anchor = actor.get_actor_location().x
        if anchor is not None:
            p = actor.get_actor_location()
            p.z += u.SkiSampleTrail.height_at(anchor)-old_height(anchor)
            actor.set_actor_location(p, False, False)
if not level.save_current_level():
    raise RuntimeError('Cannot save refreshed map')
u.log('SKI_SLOPE_REFRESHED_SCENERY_PRESERVED')
