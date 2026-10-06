/* Loop form of testcase.c, closer to Blender's BVH2::pack_instances.
   Expected: 2 4 0x7f007f 1. With gcc 14.2.0 -O2: 2 4 0x0 0 and abort. */
#include <stdio.h>
#include <stdlib.h>

typedef int v4si __attribute__((vector_size(16)));
typedef union { v4si v; struct { int x, y, z, w; } s; } int4;

__attribute__((noinline)) void merge(int4 *dst, const int4 *src, long n, int off)
{
  for (long i = 0; i < n; i++) {
    int4 d = src[i];
    d.s.x += off;
    d.s.y += off;
    dst[i].v = d.v;
  }
}

int main(void)
{
  static int4 src[4], dst[4];
  for (int i = 0; i < 4; i++) {
    src[i].s.x = 0; src[i].s.y = 2; src[i].s.z = 0x7f007f; src[i].s.w = 1;
  }
  merge(dst, src, 4, 2);
  printf("%d %d 0x%x %d\n", dst[0].s.x, dst[0].s.y, dst[0].s.z, dst[0].s.w);
  fflush(stdout);
  if (dst[0].s.z != 0x7f007f || dst[0].s.w != 1) abort();
  return 0;
}
