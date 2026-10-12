import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
assert levels.load_level('/Game/Ski25D/Maps/SkiRimeMap')
trail=next(a for a in actors.get_all_level_actors() if isinstance(a,u.SkiSampleTrail))
trail.rebuild_course()
assert levels.save_current_level()
u.log('SKI_FINISH_CLEARING_READY')
