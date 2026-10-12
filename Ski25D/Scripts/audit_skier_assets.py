import unreal as u
from pathlib import Path
import json
root=Path(u.Paths.convert_relative_path_to_full(u.Paths.project_dir()))
report={}
for name in ['FlagCloth','FlagStars','Ski','Jacket','UpperArm']:
    box=u.load_asset('/Game/Ski25D/Skier/SM_'+name).get_bounding_box()
    report[name]={'min':[box.min.x,box.min.y,box.min.z],'max':[box.max.x,box.max.y,box.max.z]}
(root/'Saved'/'SkiQA'/'skier_mesh_bounds.json').write_text(json.dumps(report,indent=2))
u.log('SKIER_BOUNDS '+json.dumps(report))
