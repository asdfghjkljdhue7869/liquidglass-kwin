#!/bin/bash
P=$HOME/projects/liquidglass-kwin
E=$P/effect
cp $E/liquidglass.cpp.pre4e $E/liquidglass.cpp
cp $E/liquidglass.h.pre4e $E/liquidglass.h
cp $E/shaders/onscreen_rounded.frag.v2 $E/shaders/onscreen_rounded.frag
ninja -C $P/build > $P/build.log 2>&1 || { echo "rebuild failed"; exit 1; }
SO=$(find $P/build -name liquidglasswash.so -type f | head -1)
cp $SO $P/stage/kwin/effects/plugins/liquidglasswash.so
sudo install -Dm755 $SO /usr/lib/qt6/plugins/kwin/effects/plugins/liquidglasswash.so
echo "rolled back 4e (run glass off first if the effect is loaded)"
