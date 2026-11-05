#!/bin/bash
d=$HOME/.config/liquidglass
n=0
until qdbus6 org.kde.KWin /Effects isEffectLoaded blur > /dev/null 2>&1; do
  n=$((n+1))
  if [ "$n" -ge 60 ]; then
    logger -t liquidglass "login: KWin effects interface not ready after 60s"
    exit 0
  fi
  sleep 1
done
sleep 3
mode=$(cat "$d/mode" 2>/dev/null)
[ -n "$mode" ] || mode=lite
if [ -f "$d/glass.pending" ]; then
  rm -f "$d/glass.pending"
  touch "$d/glass.off"
  logger -t liquidglass "login: previous login did not finish cleanly, glass switched off, delete $d/glass.off to re-enable"
fi
if [ -f "$d/round.pending" ]; then
  rm -f "$d/round.pending"
  touch "$d/round.off"
  logger -t liquidglass "login: previous login did not finish cleanly, rounding switched off, delete $d/round.off to re-enable"
fi
if [ -f "$d/glass.off" ]; then
  logger -t liquidglass "login: glass.off exists, staying on stock blur"
  exit 0
fi
guard=0
if [ "$mode" != "off" ]; then
  touch "$d/glass.pending"
  if [ ! -f "$d/round.off" ]; then touch "$d/round.pending"; fi
  guard=1
fi
out=$("$d/hooks.d/50-kwin-glass" "$mode" 2>&1)
logger -t liquidglass "login: mode $mode, hook said: ${out:-ok}"
case "$out" in
  *"rebuild first"*|*"plugin not installed"*)
    rm -f "$d/glass.pending" "$d/round.pending"
    guard=0
    ;;
esac
if [ "$guard" = "1" ]; then
  sleep 8
  if [ "$(qdbus6 org.kde.KWin /Effects isEffectLoaded liquidglasswash 2>/dev/null)" = "true" ]; then
    rm -f "$d/glass.pending"
    logger -t liquidglass "login: glass still loaded after 8s, stable"
  else
    logger -t liquidglass "login: glass not loaded after 8s, leaving the guard in place"
  fi
  if [ "$(qdbus6 org.kde.KWin /Effects isEffectLoaded liquidglassround 2>/dev/null)" = "true" ]; then
    rm -f "$d/round.pending"
    logger -t liquidglass "login: round effect still loaded after 8s, stable"
  fi
fi
exit 0
