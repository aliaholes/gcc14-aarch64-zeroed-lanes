// C++ form using the same int4 layout as Blender Cycles on aarch64 (sse2neon):
// a 16-byte union of a 128-bit vector and int x, y, z, w, with operator= copying the vector.
// The loop body is Cycles' BVH2::pack_instances leaf merge (BVH_NODE_LEAF_SIZE is 1 there).
// Expected: 2 4 0x7f007f 1 four times, then OK. With g++ 14.2.0 -O2: 2 4 0x0 0, MISCOMPILED, exit 1.
#include <arm_neon.h>
#include <cstdio>
#include <cstddef>
struct alignas(16) int4 {
  union { int32x4_t m128; struct { int x, y, z, w; }; };
  int4() = default;
  int4(const int4 &a) = default;
  int4 &operator=(const int4 &a) { m128 = a.m128; return *this; }
};
#define LEAF 1
__attribute__((noinline)) void merge(int4 *dst, size_t dst_off, const int4 *src, size_t n, size_t prim_offset)
{
  for (size_t i = 0; i < n; i += LEAF) {
    int4 data = src[i];
    data.x += prim_offset;
    data.y += prim_offset;
    dst[dst_off] = data;
    for (int j = 1; j < LEAF; ++j) dst[dst_off + j] = src[i + j];
    dst_off += LEAF;
  }
}
int main() {
  static int4 src[4], dst[8];
  for (int i = 0; i < 4; i++) { src[i].x = 0; src[i].y = 2; src[i].z = 0x7f007f; src[i].w = 1; }
  merge(dst, 2, src, 4, 2);
  int bad = 0;
  for (int i = 2; i < 6; i++) { printf("%d %d 0x%x %d\n", dst[i].x, dst[i].y, dst[i].z, dst[i].w); bad |= dst[i].z != 0x7f007f || dst[i].w != 1; }
  puts(bad ? "MISCOMPILED" : "OK"); return bad;
}
