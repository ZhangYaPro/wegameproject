"""Replace only the mountain mesh section; retain signs and other scene edits."""
import unreal as u
level=u.get_editor_subsystem(u.LevelEditorSubsystem)
if not level.load_level('/Game/Ski25D/Maps/SkiChallengeMap'):
    raise RuntimeError('Cannot load map')
trails=[a for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
        if isinstance(a,u.SkiSampleTrail)]
if len(trails)!=1:
    raise RuntimeError('Expected one trail')
trails[0].rebuild_backdrop()
if not level.save_current_level():
    raise RuntimeError('Cannot save connected mountains')
u.log('SKI_CONNECTED_MOUNTAINS_SAVED')
