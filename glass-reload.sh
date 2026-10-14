#!/bin/bash
qdbus6 org.kde.KWin /Effects reconfigureEffect liquidglasswash
echo "reconfigured: edit ~/.config/liquidglass/onscreen_rounded.frag and run this again"
