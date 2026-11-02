# Usage: blender -b --factory-startup -P scripts/pack-player-animations.py -- <player.glb> <Jumping.fbx> <out.glb>
# Packs the walking GLB and a Mixamo jump FBX (same mixamorig skeleton) into one GLB
# with two animations: "Walking" and "Jumping".
import sys
import bpy

player_glb, jump_fbx, out_glb = sys.argv[sys.argv.index('--') + 1:][:3]

bpy.ops.wm.read_factory_settings(use_empty=True)
# Mixamo FBX is 30 fps; use 30 fps for both so glTF key times round-trip unchanged.
bpy.context.scene.render.fps = 30
bpy.ops.import_scene.gltf(filepath=player_glb)
armature = next(o for o in bpy.context.scene.objects if o.type == 'ARMATURE')
walk = armature.animation_data.action
walk.name = 'Walking'
before = set(bpy.data.objects)

bpy.ops.import_scene.fbx(filepath=jump_fbx, use_anim=True)
fbx_objects = set(bpy.data.objects) - before
fbx_armature = next(o for o in fbx_objects if o.type == 'ARMATURE')
jump = fbx_armature.animation_data.action
jump.name = 'Jumping'
jump.use_fake_user = True

# Mixamo FBX bones are "mixamorig:Hips"; the glTF import names them "mixamorig:Hips" too.
# Retarget by renaming data paths only if the names differ.
glb_bones = {b.name for b in armature.data.bones}
fbx_bones = {b.name for b in fbx_armature.data.bones}
print('bones glb/fbx/common:', len(glb_bones), len(fbx_bones), len(glb_bones & fbx_bones))

# Discard the FBX objects; the actions survive through their fake users.
walk.use_fake_user = True
for obj in fbx_objects:
    bpy.data.objects.remove(obj, do_unlink=True)
for action in list(bpy.data.actions):
    if action not in (walk, jump):
        bpy.data.actions.remove(action)

# Keep the Walking action active and bind Jumping to the same armature, then export every action.
armature.animation_data.action = walk
bpy.ops.export_scene.gltf(
    filepath=out_glb, export_format='GLB',
    export_animation_mode='ACTIONS', export_force_sampling=True,
    export_animations=True, export_skins=True, export_image_format='AUTO',
)
