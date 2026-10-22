#!/bin/bash
P=$HOME/projects/liquidglass-kwin
T=/tmp/lgtest
if [ -z "$WAYLAND_DISPLAY" ]; then echo "no WAYLAND_DISPLAY: run this inside the real Plasma Wayland session"; exit 1; fi
if [ ! -x $P/testclient/glasstest-toggle ]; then echo "missing toggle client"; exit 1; fi
for f in liquidglasswash liquidglassround; do
  if [ ! -f $P/stage/kwin/effects/plugins/$f.so ]; then echo "missing staged $f"; exit 1; fi
done
rm -rf $T
mkdir -p $T/config $T/cache
[ -e $HOME/.config/kdeglobals ] && cp $HOME/.config/kdeglobals $T/config/
[ -d $HOME/.config/Kvantum ] && cp -r $HOME/.config/Kvantum $T/config/
printf "[Plugins]\nblurEnabled=false\nliquidglasswashEnabled=true\nliquidglassroundEnabled=true\nwobblywindowsEnabled=true\n" > $T/config/kwinrc
printf "[Effect-liquidglasswash]\nNoiseStrength=0\n" >> $T/config/kwinrc
printf "BlurStrength=1\n" >> $T/config/kwinrc
cat > $T/session.sh <<"INNER"
#!/bin/bash
export QT_QPA_PLATFORM=wayland
T=/tmp/lgtest
( sleep 6; qdbus6 org.kde.KWin /Effects unloadEffect liquidglassround > $T/unload.txt 2>&1; echo "unload requested at $(date +%T)" >> $T/unload.txt ) &
exec $HOME/projects/liquidglass-kwin/testclient/glasstest-toggle
INNER
chmod +x $T/session.sh
export XDG_CONFIG_HOME=$T/config XDG_CACHE_HOME=$T/cache
export QT_PLUGIN_PATH=$P/stage${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
timeout 300 dbus-run-session -- kwin_wayland --wayland-display $WAYLAND_DISPLAY --socket liquidtest --width 1280 --height 720 --scale 1 --xwayland --no-lockscreen --no-global-shortcuts --exit-with-session $T/session.sh > $T/kwin.log 2>&1
rc=$?
echo "kwin exit code: $rc"
exit $rc
