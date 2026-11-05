#!/bin/bash
d=$HOME/.config/liquidglass
t=$1
case "$t" in
  lite|full|off) ;;
  *) echo "usage: tier.sh lite|full|off"; exit 1;;
esac
if [ "$t" = "off" ]; then
  printf "off\n" > "$d/mode"
  "$d/hooks.d/50-kwin-glass" off
  exit 0
fi
conf=$d/tiers/$t.conf
[ -f "$conf" ] || { echo "missing $conf"; exit 1; }
printf "%s\n" "$t" > "$d/mode"
args=""
while read -r kind key val; do
  case "$kind" in
    kwin) kwriteconfig6 --file kwinrc --group Effect-liquidglasswash --key "$key" "$val";;
    shader) args="$args $key $val";;
  esac
done < "$conf"
"$d/hooks.d/50-kwin-glass" "$t"
if [ -n "$args" ]; then
  "$d/tune.sh" $args
else
  qdbus6 org.kde.KWin /Effects reconfigureEffect liquidglasswash
fi
echo "tier: $t"
