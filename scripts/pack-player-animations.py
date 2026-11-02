# Usage: blender -b --factory-startup -P scripts/pack-player-animations.py -- <player.glb> <out.glb> [Name=path.fbx ...]
# Adds Mixamo FBX animations (same mixamorig skeleton) to the player GLB as actions named Name,
# keeping every action the input GLB already has. Example:
#   blender -b --factory-startup -P scripts/pack-player-animations.py -- \
#     assets/models/vivi_manga_student_girl_style.glb /tmp/out.glb Idle="/path/Standing Idle.fbx"
# The game expects the actions Walking, Jumping and Idle.
import sys
import bpy

args = sys.argv[sys.argv.index('--') + 1:]
player_glb, out_glb = args[:2]
additions = [arg.split('=', 1) for arg in args[2:]]

bpy.ops.wm.read_factory_settings(use_empty=True)
# Mixamo FBX is 30 fps; use 30 fps so glTF key times round-trip unchanged.
bpy.context.scene.render.fps = 30
bpy.ops.import_scene.gltf(filepath=player_glb)
armature = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
kept = {}
for action in bpy.data.actions:
    action.use_fake_user = True
    kept[action.name] = action
print('kept actions:', sorted(kept))

for name, fbx in additions:
    before_objects = set(bpy.data.objects)
    before_actions = set(bpy.data.actions)
    bpy.ops.import_scene.fbx(filepath=fbx, use_anim=True)
    fbx_objects = set(bpy.data.objects) - before_objects
    fbx_armature = next(o for o in fbx_objects if o.type == 'ARMATURE')
    action = fbx_armature.animation_data.action
    glb_bones = {b.name for b in armature.data.bones}
    fbx_bones = {b.name for b in fbx_armature.data.bones}
    print(name, 'bones glb/fbx/common:', len(glb_bones), len(fbx_bones), len(glb_bones & fbx_bones))
    action.use_fake_user = True
    for obj in fbx_objects:
        bpy.data.objects.remove(obj, do_unlink=True)
    # Drop any other action the FBX import brought in; the wanted one survives through its fake user.
    for other in set(bpy.data.actions) - before_actions - {action}:
        bpy.data.actions.remove(other)
    action.name = name
    kept[name] = action

# Export every action (glTF ACTIONS mode); the active one is Walking when present.
if 'Walking' in kept:
    armature.animation_data.action = kept['Walking']
bpy.ops.export_scene.gltf(
    filepath=out_glb, export_format='GLB',
    export_animation_mode='ACTIONS', export_force_sampling=True,
    export_animations=True, export_skins=True, export_image_format='AUTO',
)
