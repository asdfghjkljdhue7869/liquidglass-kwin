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

## Session log 2026-10-17 to 2026-10-21 (supersedes older status where it conflicts)

### What exists now
- effect/ : liquidglasswash plugin (fork of the KWin 6.8 blur). Window glass, rim shader override, frame glass behind decorations, forced glass for popups and Plasma popups (policy file force_glass), per-kind shader constants (kind uniform: 0 window, 1 popup or menu, 2 dock).
- effect-round/ : liquidglassround plugin (OffscreenEffect, chain position 75, after wobbly at 70). Rounds window frames (kind 1), popups and menus (kind 2), floating docks (kind 3, with inset). Shadow-safe mask shader derived from KWin base.frag (roundshader.h). Settings: ~/.config/liquidglass/round.conf and frame_radius.
- install/ : live copies of files that really live in ~/.config/liquidglass (shader, tune.sh, hook 50-kwin-glass, login script), examples of the small policy files, and the edited Kvantum theme config.
- nested.sh and nested-regress.sh : nested KWin crash tests. glass-reload.sh : reconfigure helper in the repo root.

### How things load
- Login: ~/.config/autostart/liquidglass-kwin.desktop runs ~/.config/liquidglass/login.sh, which calls hooks.d/50-kwin-glass with the mode file (lite). The hook loads liquidglasswash and liquidglassround only if ~/.config/liquidglass/kwin-effect.version equals the installed kwin package version, otherwise it stays on stock blur.
- Crash guard: login.sh creates round.pending before loading and removes it 25 s later if the round effect is still loaded. A surviving round.pending at the next login creates round.off, which disables the round effect until it is deleted.
- KWin keeps the first loaded copy of a plugin in memory, so a new .so needs a logout and login. A KWin crash on Wayland ends the whole session.

### Lessons learned
- Run nested.sh and nested-regress.sh before installing anything that loads in the real session. The old round build crashed 3 of 3 nested runs (exit 139) because Qt connections used the EffectWindow as context object. Connect with this as context and guard windows with QPointer.
- A log line proves that code ran, not that something is visible. Compare screenshots.
- A KF6 update invalidates the build directory: use cmake --fresh.
- Qt debug streams insert spaces between items: never put a trailing space inside a log literal that a gate greps for.
- Kvantum: translucent_windows must stay true so menus have an alpha channel (glass menus). Window interiors are made opaque black by editing the theme svg elements window-, toolbar-, dock-, menubar-, statusbar- (fill 242424 or 333333 to 000000, window-normal opacity 0.75 to 1) and the palette colors in the kvconfig. Menu elements stay translucent.
- Theme edits applied (backups end in .pre-glass or .pre-black): Aurorae Sweet-Mars-transparent decoration.svg frame opacity 0.82 and 0.83 to 0.12; Plasma theme Sweet-mars panel-background.svgz and dialogs/background.svgz opacity 0.75 to 0.12. Kvantum theme mac-tahoe-liquid-kdeDark is installed from ~/macos-tahoe-liquid-kde and selected in ~/.config/Kvantum/kvantum.kvconfig. kdeglobals widgetStyle is kvantum.
- Shader: live reload with tune.sh or qdbus6 org.kde.KWin /Effects reconfigureEffect liquidglasswash. Constants FROST, SAT, RIM_FROST, RIM_TINT, WASH_BASE, TOP_SPEC for windows and MENU_FROST, MENU_WASH, MENU_DIM for menus. The menu glass settings were accepted by the user. A flat mode for maximized windows was tried and rejected; TOP_SPEC tones down the top rim instead.

### Open items, in the order the user cares
1. Verify global rounding (kinds 2 and 3) after login: screenshots of a desktop menu, a dock menu, the tray popup and the bottom dock. Tune round.conf (menu radius, dock radius and inset, tooltips) and run reconfigureEffect liquidglassround. The dock inset of 10 px is an estimate from one screenshot.
2. A blinking glass at the top of the screen was seen once and stopped after hovering the mouse. Not reproduced, not diagnosed.
3. Glass is hidden while wobbly windows deform a window (shouldBlur refuses transformed windows). Agreed direction: own deformation with the glass baked into the window texture, then a water-drop merge of two windows. Nothing built yet.
4. Context menus appear instantly. The user wants an animation (pop and fade). Fading and sliding popups effects are loaded, animation factor is 0.5.
5. Lag or jitter was reported once during translucent Kvantum tests. A measuring block was written but its output was never collected.
6. GTK and Brave menus, tooltips (round.conf tooltips 1, force_glass tooltip) and notifications are not covered.
7. The overall glass look is accepted.

Status when this log was written: the global rounding plugins were installed on 2026-10-22 and still need verification after a login.
