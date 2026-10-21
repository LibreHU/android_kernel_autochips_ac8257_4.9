#!/bin/sh
# Compile bootmenu (aarch64, statique, sans libc). Regenerer l'interface : python3 gen_ui.py [secondes]
set -e
cd "$(dirname "$0")"
[ -f ui_data.h ] || python3 gen_ui.py
clang --target=aarch64-linux-gnu -O2 -ffreestanding -fno-stack-protector -nostdlib -static \
      -fno-builtin -fuse-ld=lld -Wl,-e,_start -Wl,--gc-sections -s -o bootmenu bootmenu.c
echo "OK : tools/bootmenu/bootmenu ($(wc -c < bootmenu) octets)"
