#!/bin/bash
r=$HOME/projects/liquidglass-kwin
cd "$r" || exit 1
nice -n 10 cmake --fresh -S effect -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DKWin_DIR=/usr/lib/cmake/KWin > /tmp/lg-auto-cmake1.log 2>&1 || exit 1
nice -n 10 cmake --fresh -S effect-round -B build-round -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DKWin_DIR=/usr/lib/cmake/KWin > /tmp/lg-auto-cmake2.log 2>&1 || exit 1
nice -n 10 ninja -C build > /tmp/lg-auto-build1.log 2>&1 || exit 1
nice -n 10 ninja -C build-round > /tmp/lg-auto-build2.log 2>&1 || exit 1
exit 0
