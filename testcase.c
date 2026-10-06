/* GCC 14 wrong code, aarch64, -O2 (also -O3).
   Expected output: 2 4 3 4. With gcc 14.2.0 -O2: 2 4 0 0 and abort.
   x/y are updated through the union's int array; the SLP vectorizer turns that into a
   V2SI add, and after register allocation the 64-bit add writes the low half of the
   Q register that holds d, which clears lanes 2 and 3 before the 128-bit store.
   Correct with gcc 13.3.0, with -O1, and with -fno-tree-slp-vectorize. */
#include <stdio.h>
#include <stdlib.h>

typedef int v4si __attribute__((vector_size(16)));
typedef union { v4si v; int a[4]; } int4;

__attribute__((noinline)) void f(int4 *dst, const int4 *src, int off)
{
  int4 d = *src;
  d.a[0] += off;
  d.a[1] += off;
  dst->v = d.v;
}

int main(void)
{
  int4 src = { .a = { 0, 2, 3, 4 } }, dst;
  f(&dst, &src, 2);
  printf("%d %d %d %d\n", dst.a[0], dst.a[1], dst.a[2], dst.a[3]);
  fflush(stdout);
  if (dst.a[2] != 3 || dst.a[3] != 4) abort();
  return 0;
}
