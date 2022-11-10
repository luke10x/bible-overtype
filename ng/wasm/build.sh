#!/bin/bash

set -e
set -o pipefail

# clang-format \
#   --dry-run \
#   --style=Chromium \
#   /mnt/src/*.c /mnt/src/*.h
#
mkdir -p /tmp/obj

emcc /mnt/src/activity.c -c \
  -o /tmp/obj/activity.bc.o

emcc /mnt/src/ctx.c -c \
  -o /tmp/obj/ctx.bc.o
#
emcc \
  -O3 \
  -s WASM=1 \
  --no-entry \
  /mnt/src/bible.c \
  /tmp/obj/activity.bc.o \
  /tmp/obj/ctx.bc.o \
  -o bible-overtype.wasm


