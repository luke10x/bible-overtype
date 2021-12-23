# Overtype Bible Chapters with this Software!!!

### How to compile for WASM

    # make sure emcc is in path:
    source ../emsdk/emsdk_env.sh 

    make clean bible.js

    python -m http.server

Now jsut navigate to: http://0.0.0.0:8000/

### How to debug?

    # Generate core with segfault running bible binary
    cat archive/resources/teve-musu.txt |  ./bible --stdin

    # Setup where the OS store cores
    ulimit -S -c unlimited
    cat /proc/sys/kernel/core_pattern
    sudo sysctl -w kernel.core_pattern=core-%e-%s-%u-%g-%p-%t

    # gdb bible core-bible-11-1000-1000-618394-1640035687

    (gdb) bt