# gcc-14 aarch64 wrong code: Blender Cycles BVH2 drops instanced objects

GCC 14.2.0 at `-O2` (and `-O3`) on aarch64 miscompiles a partial update of a 16-byte
union that holds a 128-bit vector. The x/y lanes are added as a 64-bit vector (`add v.2s`),
which on AArch64 clears the upper half of the Q register, and the following 128-bit store
writes zeros into lanes z and w.

In Blender this hits `ccl::BVH2::pack_instances` (`intern/cycles/bvh/bvh2.cpp`). Every
instanced mesh (tested: a mesh with more than one user, and a collection instance; geometry
nodes instances take the same path in the code but were not run) renders as nothing with the BVH2 layout, which is the layout the CUDA backend
uses. Embree and OptiX are not affected.

## Files

| file | what |
|---|---|
| `testcase.c` | 20-line C testcase. Prints `2 4 3 4` when correct, `2 4 0 0` and aborts when miscompiled |
| `testcase-loop.c` | the same as a loop, closer to the Blender code |
| `testcase-cycles-int4.cpp` | C++ with the Cycles `int4` layout and the real loop shape |
| `run.sh` | builds and runs all three across gcc-14 / gcc-13 and flag variants, then shows the asm |
| `bvh2.ii.gz` | Blender 5.2.2 `bvh2.cpp`, preprocessed with g++ 14.2.0 (paths shortened) |
| `asm/` | asm excerpts (testcase and Blender `pack_instances`), GIMPLE and RTL dumps |
| `blender/min_repro.py` | Blender script: two objects sharing one cube mesh, BVH2 vs Embree |
| `blender/cycles-bvh2-pack-instances-leaf-int4.patch` | the Blender-side change that avoids the pattern |

## Result on my machine (aarch64, Ubuntu 24.04)

| compiler | -O1 | -O2 | -O3 | -O2 -fno-tree-slp-vectorize |
|---|---|---|---|---|
| gcc 14.2.0 (Ubuntu 14.2.0-4ubuntu2~24.04.1) | OK | WRONG | WRONG | OK |
| gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1) | OK | OK | OK | OK |

```
$ gcc-14 -O2 testcase.c && ./a.out
2 4 0 0
Aborted (core dumped)
```

gcc 14 `-O2` code for `f`:

```
ldr  q31, [x1]
dup  v30.2s, w2
add  v31.2s, v30.2s, v31.2s   // writes D31, clears the top 64 bits of Q31
str  q31, [x0]                // stores lanes 2 and 3 as zero
```

Newer GCC versions (14.3, 15.x, trunk) are not tested here.

## Run

```
./run.sh                      # gcc-14 and gcc-13 if installed
CCS="gcc-14 gcc-15" ./run.sh  # pick compilers
```

Blender repro (needs a Blender build made with gcc 14 on aarch64):

```
blender -b --factory-startup --python-exit-code 1 -P blender/min_repro.py -- BVH2 out.png
```

It prints a `REPRO {...}` line. `"lit": 0` means the instanced cubes vanished; 2048 is correct.

## Licence

The testcases, `run.sh` and `blender/min_repro.py` are mine and free to use for anything (CC0).
`bvh2.ii.gz` and the patch are derived from Blender's source (Cycles is Apache-2.0, the rest of
Blender GPL-2.0-or-later) and keep those licences.

Alia Holes
