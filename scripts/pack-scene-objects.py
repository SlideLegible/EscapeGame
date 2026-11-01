#!/usr/bin/env python3
"""把 Sketchfab 原始下載壓成遊戲用的場景物件 GLB：assets/models/objects/sNN.glb。

只取需要的節點、減面（Decimate Collapse）、貼圖縮到上限，再用 Blender 輸出內嵌貼圖的 GLB。
開發用工具，不屬於遊戲執行階段。需要 Blender。

用法：blender -b -P scripts/pack-scene-objects.py -- <原始 glb 資料夾> <輸出資料夾> [S01 S02 ...]
"""
import sys
import os
import bpy

# keep：只保留「自己或父節點名稱」符合的網格物件（子字串比對；空白代表全部）。
# faces：減面後的目標三角面數（None 代表不減）。
# texture：貼圖最長邊上限。
CONFIG = {
    'S01': {'keep': [], 'faces': 12000, 'texture': 1024},
    'S02': {'keep': ['TrashCan_Grey'], 'faces': None, 'texture': 512},
    'S03': {'keep': ['Vending Machine_5'], 'faces': 8000, 'texture': 1024},
    'S06': {'keep': [], 'faces': None, 'texture': 1024},
    'S07': {'keep': [], 'faces': None, 'texture': 512},
    'S10': {'keep': [], 'faces': 3000, 'texture': 512},
    'S17': {'keep': ['BoardGeo.012_4'], 'faces': 4000, 'texture': 512},
    'S15': {'keep': [], 'faces': None, 'texture': 512},
    'S18': {'keep': [], 'faces': None, 'texture': 512},
}


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def pack(sid, src, dest):
    cfg = CONFIG[sid]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)

    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH']
    if cfg['keep']:
        def named(o):
            return o.name + (o.parent.name if o.parent else '')
        drop = [o for o in meshes if not any(k in named(o) for k in cfg['keep'])]
        for o in drop:
            bpy.data.objects.remove(o, do_unlink=True)
        meshes = [o for o in meshes if o not in drop]
    if not meshes:
        raise SystemExit(f'{sid}：沒有可輸出的網格')

    before = sum(tri_count(o) for o in meshes)
    if cfg['faces'] and before > cfg['faces']:
        ratio = cfg['faces'] / before
        for o in meshes:
            mod = o.modifiers.new('decimate', 'DECIMATE')
            mod.decimate_type = 'COLLAPSE'
            mod.ratio = ratio
            mod.use_collapse_triangulate = True
    deps = bpy.context.evaluated_depsgraph_get()
    after = sum(len(o.evaluated_get(deps).data.polygons) for o in meshes)

    limit = cfg['texture']
    for image in bpy.data.images:
        if image.users and max(image.size) > limit:
            scale = limit / max(image.size)
            image.scale(max(1, int(image.size[0] * scale)), max(1, int(image.size[1] * scale)))

    # 空節點留著沒關係，但沒有網格的空節點會讓外框變大，所以一併清掉。
    for o in list(bpy.context.scene.objects):
        if o.type != 'MESH' and not o.children:
            bpy.data.objects.remove(o, do_unlink=True)

    os.makedirs(os.path.dirname(dest), exist_ok=True)
    bpy.ops.export_scene.gltf(
        filepath=dest, export_format='GLB', export_apply=True, export_image_format='AUTO',
        export_yup=True, export_cameras=False, export_lights=False,
    )
    print(f'PACK {sid}: {before} -> {after} tris, {os.path.getsize(dest) // 1024} KB -> {dest}')


def main():
    args = sys.argv[sys.argv.index('--') + 1:]
    raw_dir, out_dir, wanted = args[0], args[1], [a.upper() for a in args[2:]]
    for sid in wanted or list(CONFIG):
        pack(sid, os.path.join(raw_dir, f'{sid.lower()}.glb'), os.path.join(out_dir, f'{sid.lower()}.glb'))


main()
