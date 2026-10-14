#!/bin/bash
H=$HOME/.config/liquidglass/hooks.d/50-kwin-glass
if [ -x $H ]; then $H off; fi
rm -f $H $HOME/.config/liquidglass/kwin-effect.version
sudo rm -f /usr/lib/qt6/plugins/kwin/effects/plugins/liquidglasswash.so
echo "rolled back 4d"
