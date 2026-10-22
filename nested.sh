#!/bin/bash
MODE=${1:-fork}
P=$HOME/projects/liquidglass-kwin
T=/tmp/lgtest
if [ -z "$WAYLAND_DISPLAY" ]; then echo "no WAYLAND_DISPLAY: run this inside the real Plasma Wayland session"; exit 1; fi
if [ ! -x $P/testclient/glasstest ]; then echo "missing $P/testclient/glasstest"; exit 1; fi
if [ "$MODE" = "fork" ] && [ ! -f $P/stage/kwin/effects/plugins/liquidglasswash.so ]; then echo "missing staged plugin"; exit 1; fi
rm -rf $T
mkdir -p $T/config $T/cache
[ -e $HOME/.config/kdeglobals ] && cp $HOME/.config/kdeglobals $T/config/
[ -d $HOME/.config/Kvantum ] && cp -r $HOME/.config/Kvantum $T/config/
if [ "$MODE" = "fork" ]; then
  printf "[Plugins]\nblurEnabled=false\nliquidglasswashEnabled=true\nliquidglassroundEnabled=true\n" > $T/config/kwinrc
else
  printf "[Plugins]\nblurEnabled=true\nliquidglasswashEnabled=false\n" > $T/config/kwinrc
fi
if [ "$MODE" = "fork" ]; then
  printf "[Effect-liquidglasswash]\nNoiseStrength=0\n" >> $T/config/kwinrc
else
  printf "[Effect-blur]\nNoiseStrength=0\n" >> $T/config/kwinrc
fi
printf "BlurStrength=1\n" >> $T/config/kwinrc
cat > $T/session.sh <<"INNER"
#!/bin/bash
export QT_QPA_PLATFORM=wayland
T=/tmp/lgtest
( sleep 5; busctl --user --full introspect org.kde.KWin /Effects 2>&1 | grep loadedEffects | tr " " "\n" | grep -E "liquidglass|blur" > $T/loaded.txt; echo done >> $T/loaded.txt ) &
exec $HOME/projects/liquidglass-kwin/testclient/glasstest
INNER
chmod +x $T/session.sh
export XDG_CONFIG_HOME=$T/config XDG_CACHE_HOME=$T/cache
export QT_PLUGIN_PATH=$P/stage${QT_PLUGIN_PATH:+:$QT_PLUGIN_PATH}
echo "mode: $MODE"
timeout 900 dbus-run-session -- kwin_wayland --wayland-display $WAYLAND_DISPLAY --socket liquidtest --width 1280 --height 720 --scale 1 --xwayland --no-lockscreen --no-global-shortcuts --exit-with-session $T/session.sh > $T/kwin.log 2>&1
echo "kwin exit code: $?"
echo "== effects loaded (after done: only the lines before it count) =="
cat $T/loaded.txt 2>&1
echo "== log lines =="
grep -i -E "liquidglass|effect.*(fail|error|could not|cannot)" $T/kwin.log | cut -c1-200 | head -10
