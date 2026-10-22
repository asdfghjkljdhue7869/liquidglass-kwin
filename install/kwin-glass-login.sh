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
if [ -f "$d/round.pending" ]; then
  rm -f "$d/round.pending"
  touch "$d/round.off"
  logger -t liquidglass "login: previous login did not finish cleanly, rounding switched off, delete $d/round.off to re-enable"
fi
guard=0
if [ ! -f "$d/round.off" ] && [ "$mode" != "off" ]; then
  touch "$d/round.pending"
  guard=1
fi
out=$("$d/hooks.d/50-kwin-glass" "$mode" 2>&1)
logger -t liquidglass "login: mode $mode, hook said: ${out:-ok}"
if [ "$guard" = "1" ]; then
  sleep 25
  if [ "$(qdbus6 org.kde.KWin /Effects isEffectLoaded liquidglassround 2>/dev/null)" = "true" ]; then
    rm -f "$d/round.pending"
    logger -t liquidglass "login: round effect still loaded after 25s, stable"
  else
    logger -t liquidglass "login: round effect not loaded after 25s, leaving the guard in place"
  fi
fi
exit 0
