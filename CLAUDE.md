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
