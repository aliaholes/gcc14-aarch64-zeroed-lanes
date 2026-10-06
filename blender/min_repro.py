"""cuda-fix MINIMAL REPRO (Oct 6 2026): two objects sharing ONE cube mesh (Cycles instancing), emissive, black world, ortho camera,
1 spp, CPU, Cycles debug_bvh_layout = BVH2 (the layout CUDA traverses) vs EMBREE. Prints REPRO {...} with the lit-pixel count.
On the GB10 5.2.2 build (gcc-14, aarch64) BVH2 renders 0 lit pixels: every instanced mesh vanishes. 4.0.2 and the patched build render them.
  blender -b --factory-startup --python-exit-code 1 -P min_repro.py -- BVH2|EMBREE OUT.png
"""
import json, sys
import bpy, bmesh
import numpy as np

layout, out = sys.argv[sys.argv.index("--") + 1:][:2]
sc = bpy.context.scene
for ob in list(bpy.data.objects):
    bpy.data.objects.remove(ob)
sc.render.engine = "CYCLES"
sc.cycles.device = "CPU"
sc.cycles.samples = 1
sc.cycles.use_adaptive_sampling = False
sc.cycles.use_denoising = False
sc.render.resolution_x, sc.render.resolution_y, sc.render.resolution_percentage = 128, 64, 100
p = bpy.context.preferences
p.experimental.use_cycles_debug = True  # exposes debug_bvh_layout
p.view.show_developer_ui = True
sc.cycles.debug_bvh_layout = layout
sc.world = bpy.data.worlds.new("w")
sc.world.color = (0, 0, 0)
mat = bpy.data.materials.new("emit")
if bpy.app.version < (5, 0, 0):
    mat.use_nodes = True
nt = mat.node_tree
for nd in list(nt.nodes):
    nt.nodes.remove(nd)
em = nt.nodes.new("ShaderNodeEmission")
nt.links.new(em.outputs[0], nt.nodes.new("ShaderNodeOutputMaterial").inputs[0])
me = bpy.data.meshes.new("cube")
bm = bmesh.new(); bmesh.ops.create_cube(bm, size=1.0); bm.to_mesh(me); bm.free()
me.materials.append(mat)
for i in range(2):  # two users of one mesh = an instanced geometry in Cycles
    ob = bpy.data.objects.new(f"inst{i}", me); ob.location = (i * 1.5, 0, 0); sc.collection.objects.link(ob)
cam = bpy.data.cameras.new("c"); cam.type = "ORTHO"; cam.ortho_scale = 4.0
co = bpy.data.objects.new("cam", cam); co.location = (0.75, -10, 0); co.rotation_euler = (1.5707963, 0, 0)
sc.collection.objects.link(co); sc.camera = co
sc.render.filepath = out
bpy.ops.render.render(write_still=True)
im = bpy.data.images.load(out)
px = np.array(im.pixels[:]).reshape(-1, 4)
lit = int((px[:, :3].max(1) > 0.5).sum())
print("REPRO " + json.dumps(dict(blender=bpy.app.version_string, build_hash=bpy.app.build_hash.decode() if isinstance(bpy.app.build_hash, bytes) else bpy.app.build_hash,
      layout=sc.cycles.debug_bvh_layout, mesh_users=me.users, lit=lit, verdict="DROP" if lit == 0 else "RENDERS")), flush=True)
