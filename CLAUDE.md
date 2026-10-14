# liquidglass-kwin

Liquid-glass KWin effect for the whole desktop. Out-of-tree fork of the KWin blur plugin (plugin id liquidglasswash).
Companion to the launcher fork asdfghjkljdhue7869/Linexin-Launcher (branch liquid-glass), whose glass_pill.frag is the approved look.

## Environment
- Dell Latitude 5410, CachyOS, Plasma 6.7.5 Wayland, Intel UHD iGPU (Lite tier). Full tier planned for the RTX 5060 Ti PC (around Christmas).
- Shell is fish. Deliver commands as ONE bash -c block, no single quotes inside, no double backslashes, heredocs inside it.
- User wants: one complete copy-pasteable block per step, anchor guards, never guess names or paths, inspect real files first, no undo commands in runnable code blocks (the user pastes everything), trial blocks run as the normal user without sudo (only the install line uses sudo).

## What is here
- effect/ : plugin source, forked from KWin v6.7.5 src/plugins/blur and renamed LiquidGlassEffect. Reference KWin source clone: kwin-src/ (not committed).
- effect/shaders/onscreen_rounded.frag : the glass look (built-in copy).
- install/ : live copies of the override shader, the hook and the login script.
- testclient/ : glasstest.cpp (backdrop plus translucent window asking for blur), nested.sh (nested KWin crash test only).
- C++ changes on top of stock blur: sharpTex (second sampler, unblurred copy on unit 1), runtime shader override from ~/.config/liquidglass/onscreen_rounded.frag at reconfigure, and routing of every window through the rounded pass (windows with no border radius use the blur region bounds and a 14 px radius).

## Build and install
- cmake -S effect -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DKWin_DIR=/usr/lib/cmake/KWin ; ninja -C build
- CMakeLists needs CMAKE_CXX_STANDARD 23, Qt6 Core Gui DBus Widgets Quick, KF6 Config CoreAddons WindowSystem, KDecoration3, KWin, XCB (link XCB::XCB).
- Install: sudo install -Dm755 build/bin/kwin/effects/plugins/liquidglasswash.so /usr/lib/qt6/plugins/kwin/effects/plugins/ and write the output of pacman -Q kwin (version field) to ~/.config/liquidglass/kwin-effect.version.
- After EVERY kwin package update: clean rebuild and reinstall. The hook refuses to load a plugin built for another kwin version and stays on stock blur.
- KWin keeps the first loaded copy of a plugin in memory: a new .so needs a logout and login. Shader override edits do not.

## Switch and tuning
- ~/.config/liquidglass/hooks.d/50-kwin-glass off|lite|full (called by the glass tool). Session-only: unloads stock blur, loads liquidglasswash over D-Bus (qdbus6 org.kde.KWin /Effects). kwinrc plugin keys are never touched. Lite and Full are identical for now.
- Login: install/kwin-glass-login.sh plus ~/.config/autostart/liquidglass-kwin.desktop (reads the mode file).
- Live tuning: edit the override shader, run glass-reload.sh (reconfigureEffect liquidglasswash). The journal line "shader override loaded" proves which file is active.
- kwinrc group [Effect-liquidglasswash]: BlurStrength 4, NoiseStrength 0, Saturation 100 (copied from the stock blur settings, plus the frost tuning).

## Approved baseline look (constants at the top of the shader)
BAND 40, LENS_PX 24, DRIFT 0.12, FROST 0.30, RIM_FROST 0.70, RIM_TINT 0.22, WASH_BASE 0.05, lens ease pow(1-t,1.6). Keep LENS_PX under about 0.6 times BAND or the lens folds and looks binary. The user approved this state as a baseline, not as final.

## Lessons learned
- Real desktop: the Aurorae decoration (Sweet-Mars-transparent) reports no border radius, so before the routing patch the glass shader never ran for those windows. Only Breeze among the installed decorations imports setBorderRadius.
- The nested session shows a 2x2 dot lattice artifact and uses different decoration radii. Use it only as a crash test; judge the look in the real session.
- Prove which shader is running (journal line, visible debug bands) before tuning anything.
- /proc/PID/maps of kwin_wayland is unreadable; use the package version and process start time instead.
- glass_pill.frag from the launcher is the reference for the look (lens only at the rim, prism lines, specular lines).

## Open ideas
- Animation: time uniform, drifting wash, liquid ripple.
- Lite and Full tiers via a uniform; with FROST 0 the blur chain could be skipped to save GPU.
- Corner radius for windows without one is hardcoded 14 px in C++; consider a shader FORCE_RADIUS or a config key. The user plans a thicker window border so the rounded glass inset looks natural.
- Force glass for windows that never request blur (Brave and others).
- Automatic rebuild after KWin package updates (pacman hook).
- Panel and popup behaviour after the effect swap is unverified.

## Next session (written at the end of session 1)
Status: the glass baseline is approved and committed. The plugin is installed and loads through the hook. A login automation script exists (install/kwin-glass-login.sh); whether the autostart entry was installed is not confirmed. The user set the window borders to huge in System Settings (Aurorae theme Sweet-Mars-transparent) so the rounded glass sits as an inset in the frame.

Immediate goals:
1. Remove the square outer corners of window frames, system-wide (the Aurorae decoration reports no radius).
2. The user will describe what still bothers them about the glass. Ask first, do not guess. Tune live with the override shader.
3. First step of the session: compare pacman -Q kwin with ~/.config/liquidglass/kwin-effect.version. A KWin update means the plugin must be rebuilt (clean rebuild, install, refresh the version file, log out and in).

Corner plan (names checked in the KWin 6.7.5 source, nothing compiled yet):
- EffectWindow::windowItem() is public. WindowItem::decorationItem() is public and KWIN_EXPORT. Item::setBorderRadius(const BorderRadius&) is public. BorderRadius lives in scene/borderradius.h (class KWIN_EXPORT BorderRadius); its constructors were not checked yet.
- The scene clips an item and all of its descendants to that items rounded rect (scene/workspacescene.cpp, calculateClipCorner). The client content container already receives the window radius. The DecorationItem and the ShadowItem are siblings of it, so a radius set on the DecorationItem rounds the frame without clipping the shadow. With huge borders the client corners stay inside the frame.
- Plan: in LiquidGlassEffect, set a radius on the decoration item of normal windows that have a decoration. Skip maximized and fullscreen windows. Handle windowAdded and the existing windows at load. Restore a null radius in the destructor and when the effect unloads.
- Not found yet: signal names for maximize, fullscreen and decoration changes. They are not in effect/effecthandler.h; look in effect/effectwindow.h and window.h before writing code. EffectsHandler::windowAdded, windowDeleted and stackingOrder() exist. EffectWindow::hasDecoration(), isFullScreen(), isNormalWindow() exist.
- Alternatives: edit the corner shapes in the Aurorae SVG of Sweet-Mars-transparent (theme files under ~/.local/share/aurorae/themes), or switch to Breeze (the only installed decoration importing setBorderRadius, but it loses the traffic-light look).
- Every plugin rebuild needs: install, version file refresh, logout and login once.
