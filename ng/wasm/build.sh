emcc \
  -O3 \
  -s WASM=1 \
  --no-entry \
  -o bible-overtype.wasm \
  /mnt/src/bible-wasm.c

