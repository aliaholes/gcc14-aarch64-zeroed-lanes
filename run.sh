#!/bin/sh
# Build and run the testcases with each compiler/flag combination.
# Usage: ./run.sh            (uses gcc-14/g++-14, and gcc-13/g++-13 if present)
#        CCS="gcc-14 gcc-15" ./run.sh
# A testcase exits 0 when lanes z/w survive, aborts (non-zero) when they are zeroed.
set -u
cd "$(dirname "$0")"
OUT=$(mktemp -d)
CCS=${CCS:-"gcc-14 gcc-13"}
for cc in $CCS; do
  command -v "$cc" >/dev/null 2>&1 || { echo "skip $cc (not installed)"; continue; }
  cxx=$(echo "$cc" | sed 's/^gcc/g++/')
  echo "== $($cc --version | head -1)"
  for opt in "-O1" "-O2" "-O3" "-O2 -fno-tree-slp-vectorize" "-O2 -fno-tree-vectorize"; do
    for t in testcase.c testcase-loop.c testcase-cycles-int4.cpp; do
      case $t in *.cpp) c=$cxx; std=-std=c++20 ;; *) c=$cc; std= ;; esac
      if $c $std $opt -o "$OUT/t" "$t" 2>"$OUT/err"; then
        if "$OUT/t" >"$OUT/out" 2>&1; then r=OK; else r=WRONG; fi
        printf '%-30s %-28s %-6s %s\n' "$t" "$opt" "$r" "$(head -1 "$OUT/out")"
      else
        printf '%-30s %-28s %s\n' "$t" "$opt" "COMPILE-FAIL"
      fi
    done
  done
  $cc -O2 -S -o "$OUT/f.s" testcase.c && echo "-- $cc -O2 asm of f():" && sed -n '/^f:/,/\.size/p' "$OUT/f.s" | grep -vE '\.cfi|^\.LF'
done
if [ -f bvh2.ii.gz ] && command -v g++-14 >/dev/null 2>&1; then
  gzip -dc bvh2.ii.gz > "$OUT/bvh2.ii"
  g++-14 -O2 -march=armv8.2-a+dotprod+fp16+lse -fPIC -funsigned-char -fno-strict-aliasing -ffp-contract=off \
    -fno-trapping-math -fno-math-errno -fno-signed-zeros -ffp-contract=on -freciprocal-math -fno-signaling-nans \
    -fno-rounding-math -std=c++20 -w -S -o "$OUT/bvh2.s" "$OUT/bvh2.ii"
  echo "-- bvh2.ii (Blender 5.2.2 bvh2.cpp, preprocessed), g++-14 -O2, pack_instances leaf-merge loop:"
  awk '/^_ZN3ccl4BVH214pack_instancesEmm:/{p=1} p{print} /\.size\t_ZN3ccl4BVH214pack_instancesEmm/{p=0}' "$OUT/bvh2.s" \
    | grep -B2 -A2 -E 'add	v[0-9]+\.2s, v[0-9]+\.2s'
fi
rm -rf "$OUT"
