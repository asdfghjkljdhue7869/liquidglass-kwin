#!/bin/bash
d=$HOME/.config/liquidglass
f=$d/onscreen_rounded.frag
r=$HOME/projects/liquidglass-kwin
[ -f "$f" ] || { echo "missing $f"; exit 1; }
if [ "$#" -lt 2 ] || [ $(( $# % 2 )) -ne 0 ]; then
  echo "usage: tune.sh NAME VALUE [NAME VALUE ...]"
  echo "names: BAND LENS_PX DRIFT FROST SAT RIM_FROST RIM_TINT WASH_BASE DITHER TOP_SPEC"
  exit 1
fi
cp "$f" "$f.last"
while [ "$#" -gt 0 ]; do
  name=$1
  val=$2
  shift 2
  case "$name" in
    BAND|LENS_PX|DRIFT|FROST|SAT|RIM_FROST|RIM_TINT|WASH_BASE|DITHER|TOP_SPEC|MENU_FROST|MENU_WASH|MENU_DIM) ;;
    *) echo "unknown constant $name"; exit 1;;
  esac
  case "$val" in
    ""|*[!0-9.]*) echo "bad value $val"; exit 1;;
  esac
  c=$(grep -c -E "^const float $name = [0-9.]+;\$" "$f")
  [ "$c" = "1" ] || { echo "constant $name not found exactly once"; exit 1; }
  sed -i -E "s/^const float $name = [0-9.]+;\$/const float $name = $val;/" "$f"
done
[ -d "$r/install" ] && cp "$f" "$r/install/onscreen_rounded.frag"
qdbus6 org.kde.KWin /Effects reconfigureEffect liquidglasswash
sleep 2
grep -n -E "^const float" "$f"
pid=$(pgrep -x kwin_wayland | head -n1)
echo "now: $(date +%T)"
journalctl -b _PID="$pid" --no-pager 2>/dev/null | grep "shader override" | tail -n 1 | cut -c1-140
