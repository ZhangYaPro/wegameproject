"""Run with UnrealEditor-Cmd -run=pythonscript -script=... -AllowCommandletRendering.
Creates only /Game/Ski25D assets; the original ThirdPersonMap stays untouched.
"""
import unreal as u
from pathlib import Path

ROOT=Path(u.Paths.project_dir()).resolve()
BASE='/Game/Ski25D'
LIB=u.MaterialEditingLibrary
ASSET=u.EditorAssetLibrary
TOOLS=u.AssetToolsHelpers.get_asset_tools()
for folder in ['Materials','Textures','Maps']:
    ASSET.make_directory(BASE+'/'+folder)

def load(path): return u.load_asset(path)
def texture(name, normal=False):
    path=BASE+'/Textures/'+name
    if not ASSET.does_asset_exist(path):
        t=u.AssetImportTask();t.set_editor_property('filename',str(ROOT/'ContentSource'/'SkiSample'/(name+'.png')))
        t.set_editor_property('destination_path',BASE+'/Textures')
        t.set_editor_property('destination_name',name);t.set_editor_property('automated',True)
        t.set_editor_property('save',True);t.set_editor_property('replace_existing',True)
        TOOLS.import_asset_tasks([t])
    tex=load(path)
    if normal:
        tex.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property('srgb',False)
    ASSET.save_asset(path)
    return tex

def node(mat,cls,x=0,y=0):return LIB.create_material_expression(mat,cls,x,y)
def constant(mat,val):
    n=node(mat,u.MaterialExpressionConstant);n.set_editor_property('r',val);return n
def material(name,color,rough=.8,metal=0,emission=0,snow=False):
    path=BASE+'/Materials/M_'+name
    if ASSET.does_asset_exist(path):
        m=load(path)
        if not m.get_editor_property('used_with_instanced_static_meshes'):
            m.set_editor_property('used_with_instanced_static_meshes',True)
            LIB.recompile_material(m)
            if not ASSET.save_asset(path):raise RuntimeError('Cannot save material: '+path)
        return m
    m=TOOLS.create_asset('M_'+name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
    m.set_editor_property('used_with_instanced_static_meshes',True)
    c=node(m,u.MaterialExpressionConstant3Vector);c.set_editor_property('constant',u.LinearColor(*color,1))
    LIB.connect_material_property(c,'',u.MaterialProperty.MP_BASE_COLOR)
    LIB.connect_material_property(constant(m,rough),'',u.MaterialProperty.MP_ROUGHNESS)
    LIB.connect_material_property(constant(m,metal),'',u.MaterialProperty.MP_METALLIC)
    if emission:
        mul=node(m,u.MaterialExpressionMultiply);LIB.connect_material_expressions(c,'',mul,'A')
        LIB.connect_material_expressions(constant(m,emission),'',mul,'B')
        LIB.connect_material_property(mul,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    if snow:
        uv=node(m,u.MaterialExpressionTextureCoordinate)
        uv.set_editor_property('u_tiling',7);uv.set_editor_property('v_tiling',7)
        sample=node(m,u.MaterialExpressionTextureSample);sample.set_editor_property('texture',texture('SnowNormal',True))
        sample.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        LIB.connect_material_expressions(uv,'',sample,'UVs')
        LIB.connect_material_property(sample,'RGB',u.MaterialProperty.MP_NORMAL)
    LIB.recompile_material(m);ASSET.save_asset(path);return m

def signmat(name):
    path=BASE+'/Materials/M_'+name
    if ASSET.does_asset_exist(path):
        m=load(path)
        if m.get_editor_property('shading_model') == u.MaterialShadingModel.MSM_UNLIT:return m
        LIB.delete_all_material_expressions(m)
    else:
        m=TOOLS.create_asset('M_'+name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
    m.set_editor_property('two_sided',True)
    m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    s=node(m,u.MaterialExpressionTextureSample);s.set_editor_property('texture',texture(name))
    mul=node(m,u.MaterialExpressionMultiply)
    LIB.connect_material_expressions(s,'RGB',mul,'A')
    LIB.connect_material_expressions(constant(m,.75),'',mul,'B')
    LIB.connect_material_property(mul,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    LIB.recompile_material(m);ASSET.save_asset(path);return m

material('Snow',(.78,.89,.94),.80,snow=True)
material('IceEdge',(.26,.48,.58),.52)
material('Bark',(.11,.065,.047),.94)
material('Cedar',(.28,.12,.062),.80)
material('Midnight',(.018,.062,.086),.43,.16)
material('Brass',(.71,.39,.12),.27,.62)
material('WarmGlass',(1,.43,.10),.4,0,2.2)
material('Pine',(.025,.115,.13),.9)
material('Stone',(.11,.18,.23),.93)
material('River',(.025,.12,.16),.24,.35)
material('Coral',(.82,.15,.055),.61)
for n in ['Sign_Start','Sign_Rime','Sign_Lodge','Sign_Finish','Sign_Changbai','Sign_Chagan','Sign_Yanbian','Sign_Tram','Sign_Festival']:signmat(n)

for name,color,emission in [('SnowFX',(.55,.7,.85),.7),('Firework0',(1,.43,.06),8),('Firework1',(.06,.7,1),8),('Firework2',(1,.08,.28),8)]:
    m=material(name,color,.8,0,emission)
    if m.get_editor_property('shading_model') != u.MaterialShadingModel.MSM_UNLIT:
        m.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
        LIB.recompile_material(m);ASSET.save_loaded_asset(m)
if not ASSET.does_asset_exist(BASE+'/Materials/M_LampGlow'):
    m=material('LampGlow',(1,.54,.16),.4)
    glow=node(m,u.MaterialExpressionScalarParameter)
    glow.set_editor_property('parameter_name','NightGlow');glow.set_editor_property('default_value',0)
    color=node(m,u.MaterialExpressionConstant3Vector);color.set_editor_property('constant',u.LinearColor(1,.43,.10,1))
    mul=node(m,u.MaterialExpressionMultiply)
    LIB.connect_material_expressions(color,'',mul,'A');LIB.connect_material_expressions(glow,'',mul,'B')
    LIB.connect_material_property(mul,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    LIB.recompile_material(m);ASSET.save_loaded_asset(m)
ASSET.make_directory(BASE+'/Audio')
if not ASSET.does_asset_exist(BASE+'/Audio/WinterWind'):
    task=u.AssetImportTask();task.set_editor_property('filename',str(ROOT/'ContentSource'/'SkiSample'/'WinterWind.wav'))
    task.set_editor_property('destination_path',BASE+'/Audio');task.set_editor_property('automated',True);task.set_editor_property('save',True)
    TOOLS.import_asset_tasks([task])
wind=load(BASE+'/Audio/WinterWind');wind.set_editor_property('looping',True)
if wind.get_editor_property('sound_asset_compression_type') != u.SoundAssetCompressionType.PCM:
    wind.set_sound_asset_compression_type(u.SoundAssetCompressionType.PCM)
ASSET.save_loaded_asset(wind)

# Materials-only pass is useful before a fresh C++ build loads their constructors.
if '-SkiMaterialsOnly' in u.SystemLibrary.get_command_line():
    u.log('SKI_MATERIALS_READY')
else:
    map_path=BASE+'/Maps/SkiChallengeMap'
    level=u.get_editor_subsystem(u.LevelEditorSubsystem)
    if ASSET.does_asset_exist(map_path):
        level.load_level(map_path)
        for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
            if not isinstance(a,(u.WorldSettings,u.Brush)):
                u.get_editor_subsystem(u.EditorActorSubsystem).destroy_actor(a)
    else:
        level.new_level(map_path)
    actors=u.get_editor_subsystem(u.EditorActorSubsystem)
    def spawn(cls,pos=(0,0,0),rot=(0,0,0),label=''):
        a=actors.spawn_actor_from_class(cls,u.Vector(*pos),u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
        if label:a.set_actor_label(label)
        return a
    def box(label,pos,scale,mat,rot=(0,0,0),shape='Cube'):
        a=spawn(u.StaticMeshActor,pos,rot,label)
        c=a.static_mesh_component;c.set_static_mesh(load('/Engine/BasicShapes/'+shape))
        c.set_material(0,load(BASE+'/Materials/M_'+mat));c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
        a.set_actor_scale3d(u.Vector(*scale));return a
    trail_cls=u.load_class(None,'/Script/Ski25D.SkiSampleTrail')
    if trail_cls is None:raise RuntimeError('Build Ski25DEditor before generating map')
    spawn(trail_cls,label='吉林雾凇 · 可玩雪道与场景')
    height=u.SkiSampleTrail.height_at
    spawn(u.PlayerStart,(200,0,height(200)+91),label='滑雪起点')
    sun=spawn(u.DirectionalLight,(0,0,1500),(-24,-38,0),'清晨金色阳光')
    sun.light_component.set_editor_property('mobility',u.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property('intensity',6.0)
    sun.light_component.set_editor_property('light_color',u.Color(255,227,196,255))
    sun.light_component.set_editor_property('light_source_angle',3.0)
    sun.light_component.set_editor_property('atmosphere_sun_light',True)
    sky=spawn(u.SkyLight,label='天空柔光')
    sky.light_component.set_editor_property('mobility',u.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property('intensity',2.0)
    sky.light_component.set_editor_property('real_time_capture',True)
    spawn(u.SkyAtmosphere,label='冬日天空')
    fog=spawn(u.ExponentialHeightFog,(0,0,-170),label='江面薄雾')
    fog.component.set_editor_property('fog_density',.008)
    fog.component.set_editor_property('fog_height_falloff',.3)
    fog.component.set_editor_property('fog_inscattering_luminance',u.LinearColor(.48,.66,.76,1))
    pp=spawn(u.PostProcessVolume,label='雪景色调')
    pp.set_editor_property('unbound',True)
    settings=pp.get_editor_property('settings')
    for key,val in [('override_auto_exposure_method',True),('auto_exposure_method',u.AutoExposureMethod.AEM_MANUAL),('override_auto_exposure_bias',True),('auto_exposure_bias',0.0),('override_auto_exposure_apply_physical_camera_exposure',True),('auto_exposure_apply_physical_camera_exposure',False),('override_bloom_intensity',True),('bloom_intensity',.22),('override_motion_blur_amount',True),('motion_blur_amount',0.0),('override_vignette_intensity',True),('vignette_intensity',.16)]:
        settings.set_editor_property(key,val)
    pp.set_editor_property('settings',settings)
    for x,name in [(900,'Sign_Start'),(10100,'Sign_Rime'),(19500,'Sign_Lodge'),(45500,'Sign_Changbai'),(55000,'Sign_Chagan'),(68000,'Sign_Yanbian'),(80500,'Sign_Tram'),(95500,'Sign_Festival'),(100400,'Sign_Finish')]:
        # Keep the entire board above the uphill edge, not just its centre.
        bottom=max(height(x+dx) for dx in range(-350,351,10))+85
        centre=bottom+167.5
        box(name+'_Back',(x,-255,centre),(6.6,.23,3.35),'Midnight')
        box(name,(x,-240,centre),(6.4,3.2,1),name,(0,0,90),'Plane')
        for dx in [-245,245]:
            foot=height(x+dx)-10
            top=bottom+30
            box(name+'_Post',(x+dx,-260,(foot+top)/2),(.17,.22,(top-foot)/100),'Cedar')
        box(name+'_Cap',(x,-255,centre+172),(6.9,.5,.12),'Snow')
    world=level.get_current_level().get_outer()
    ws=world.get_world_settings()
    ws.set_editor_property('default_game_mode',u.load_class(None,'/Script/Ski25D.SkiSampleGameMode'))
    ws.set_editor_property('kill_z',-40000)
    u.get_editor_subsystem(u.UnrealEditorSubsystem).set_level_viewport_camera_info(u.Vector(3100,3300,height(3100)+1000),u.Rotator(pitch=-12,yaw=-90,roll=0))
    if not level.save_current_level():raise RuntimeError('Cannot save map: '+map_path)
    u.log('SKI_MAP_READY '+map_path)
