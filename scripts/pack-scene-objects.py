#!/usr/bin/env python3
"""把 Sketchfab 原始下載壓成遊戲用的場景物件 GLB：assets/models/objects/sNN.glb。

只取需要的節點、減面（Decimate Collapse）、貼圖縮到上限，再用 Blender 輸出內嵌貼圖的 GLB。
開發用工具，不屬於遊戲執行階段。需要 Blender。

用法：blender -b -P scripts/pack-scene-objects.py -- <原始 glb 資料夾> <輸出資料夾> [S01 S02 ...]
"""
import sys
import os
import bpy
import numpy as np

# keep：只保留「自己或父節點名稱」符合的網格物件（子字串比對；空白代表全部）。
# faces：減面後的目標三角面數（None 代表不減）。
# texture：貼圖最長邊上限。
# mark：在底色貼圖上畫一個紅圈，(x, y) 是從左上角算起的像素座標（縮圖之前的原始尺寸）。
CONFIG = {
    'S01': {'keep': [], 'faces': 12000, 'texture': 1024},
    'S02': {'keep': ['TrashCan_Grey'], 'faces': None, 'texture': 512},
    'S03': {'keep': ['Vending Machine_5'], 'faces': 8000, 'texture': 1024},
    'S06': {'keep': [], 'faces': None, 'texture': 1024},
    'S07': {'keep': [], 'faces': None, 'texture': 512},
    'S08': {'keep': [], 'faces': None, 'texture': 512},
    'S10': {'keep': [], 'faces': 3000, 'texture': 512},
    'S17': {'keep': ['BoardGeo.012_4'], 'faces': 4000, 'texture': 512},
    'S12': {'keep': [], 'faces': None, 'texture': 512},
    'S13': {'keep': [], 'faces': None, 'texture': 512},
    'S15': {'keep': [], 'faces': None, 'texture': 512},
    # 貼圖是轉 90° 並鏡像存放的；10 月那塊的 31 號在 (113, 471)。
    'S18': {'keep': [], 'faces': None, 'texture': 1024, 'mark': {'x': 113, 'y': 471, 'radius': 9, 'width': 2.2}},
}


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def base_color_image(meshes):
    for obj in meshes:
        for slot in obj.material_slots:
            tree = slot.material.node_tree if slot.material else None
            if not tree:
                continue
            for node in tree.nodes:
                if node.type == 'BSDF_PRINCIPLED':
                    link = node.inputs['Base Color'].links
                    if link and link[0].from_node.type == 'TEX_IMAGE':
                        return link[0].from_node.image
    return None


def draw_ring(image, x, y, radius, width, color=(0.86, 0.08, 0.08)):
    w, h = image.size
    px = np.array(image.pixels[:], dtype=np.float32).reshape(h, w, 4)
    row = h - 1 - y  # Blender 的像素列由下往上
    yy, xx = np.mgrid[0:h, 0:w]
    dist = np.hypot(xx - x, yy - row)
    cover = np.clip(width / 2 + 0.5 - np.abs(dist - radius), 0, 1)[..., None]
    px[..., :3] = px[..., :3] * (1 - cover) + np.array(color, dtype=np.float32) * cover
    image.pixels = px.reshape(-1).tolist()
    image.update()


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

    mark = cfg.get('mark')
    if mark:
        image = base_color_image(meshes)
        if image is None:
            raise SystemExit(f'{sid}：找不到底色貼圖，無法畫圈')
        draw_ring(image, mark['x'], mark['y'], mark['radius'], mark['width'])

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
