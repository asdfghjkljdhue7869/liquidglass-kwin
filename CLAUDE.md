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

## Session log 2026-10-27 to 2026-11-01 (supersedes older status where it conflicts)

### Status
- liquidglassround rounds three kinds of windows: 1 decorated frames, 2 menus and popups, 3 the floating dock (with an inset). Chain position 75, after wobbly. The shadow-safe mask shader keeps shadows outside the rectangle (version 1).
- Tier switch: ~/.config/liquidglass/tier.sh lite|full|off applies tiers/lite.conf or tiers/full.conf (kwinrc BlurStrength, NoiseStrength, Saturation plus shader constants through tune.sh). Lite is the approved iGPU look. Full (stronger blur, grain, lens, dispersion) is a first guess. The mode file records the last tier.
- The SSD moves between the Dell laptop (Intel iGPU, use lite) and a desktop (Ryzen 5 5600X, RTX 3060 12 GB, NVIDIA open driver 615, KWin on OpenGL ES, HDMI 1080p at scale 1.35, use full). An auto tier by GPU at login is not built yet.
- Hook 50-kwin-glass skips the glass load if liquidglasswash is already loaded. It used to treat that as a failed load and bring stock blur back, which also skipped the round effect.
- Automatic rebuild after kwin upgrades, VERIFIED on 2026-11-05: pacman -Syu upgraded kwin 6.8.2-1.1 to 6.8.2-2.1, the hook ran, rebuilt and installed the plugins, and the upgrade took about 1.5 minutes. Pacman hook /etc/pacman.d/hooks/liquidglass-rebuild.hook runs /usr/local/bin/liquidglass-rebuild, which builds as the user through install/lg-build.sh (cmake --fresh and ninja for effect and effect-round), checks both plugins with ldd -r, installs them to /usr/lib/qt6/plugins/kwin/effects/plugins, writes kwin-effect.version and logs to ~/.config/liquidglass/rebuild.log. If anything fails nothing is installed and the version guard keeps stock blur. Run it by hand with sudo /usr/local/bin/liquidglass-rebuild. Copies of all files are in install/.
- Login guards in login.sh, VERIFIED at a real login on 2026-11-05 (glass and round effect stable after 8 s): glass.pending and round.pending are created before loading and removed 8 seconds later if the effect is still loaded. A surviving marker at the next login creates glass.off or round.off, delete the file to re-enable. A refused hook (rebuild first) clears the markers.
- NOT yet verified: the plugins built for kwin 6.8.2-2.1 loading after a restart. Check first.

### Findings
- NVIDIA desktop: the launcher shows no glass there. plasmashell logged about 10780 times Failed to compile shader: 0(5) : error C7573: OpenGL/ES requires precision specifier on this float type in the NVIDIA boot and nothing in the Intel boot. On the Intel laptop the launcher glass works. The launcher glass does not depend on the KWin plugin. Likely cause: a launcher shader has float declarations without precision in its OpenGL ES variant. Next step: /usr/lib/qt6/bin/qsb -d on each .qsb to dump the ES variant, find the shader, fix the source and rebuild the qsb.
- Thin dark straight lines along the bottom and side edges of rounded menus and the dock, stopping where the rounded corner starts. Not on decorated frames. Believed to be the original rectangular shadow that the mask keeps outside the rectangle while the body is rounded.
- About 5 percent leftover transparency at rounded corners, and the dock looks like two plates (a fainter plate around the last icon): not diagnosed, need zoomed screenshots on a bright background.
- The window titlebar right-click menu has no glass. It is the KWin own menu. Capture its window flags with a KWin script before forcing glass on it.

### Open items in order
1. Launcher shader precision fix for NVIDIA.
2. Shadow-safe shader version 2: remove the original shadow and draw a rounded soft shadow from the rounded-box distance, strength, size and y offset in round.conf.
3. Titlebar menu glass.
4. Auto tier by GPU at login.
5. Corner transparency and the dock plate.
6. Later: animated context menus, a blinking glass at the top of the screen seen once, wobbly plus glass (own deformation, then a water-drop merge of two windows), lag numbers never collected.

## Session log 2026-11-05 to 2026-11-08 (supersedes older status where it conflicts)

### Verified (log line or screenshot)
- Automatic rebuild after kwin 6.8.2-2.1: version file matched, both effects loaded, login guards stable.
- Titlebar (window operations) menu: a KWin internal QMenu (popupWindow true, empty class and caption). Glass was already drawn behind it, but the body was opaque. A widget probe showed translucent false; a grab probe showed alpha ff with fill 333333. Fix: the kwinmenu word in force_glass makes an application event filter in the glass plugin set WA_TranslucentBackground on QMenu top-levels at Polish and Show; in the Kvantum theme svg the three menu-normal fill-opacity 1 entries (menu-normal, menu-normal-left, menu-normal-topleft) went to 0. Confirmed by screenshot.
- Black 1 px line around menus: the menu surface has a 4 px ring (frame 447x256, blur region inner 4,4 439x248, Kvantum menu_shadow_depth 4). The round mask now uses the inner rect (bounding rect of the surface blur region) for menus (kind 2) and the dock (kind 3), and the frame rect for decorated frames (kind 1). Outside the rounded rect the original shadow is replaced by a rounded one from the shadow row in round.conf (strength size yoffset); strength 0 means no shadow. Dock body inner rect 8,8 653x71 in a 669x87 frame.
- Dock: the glass plugin now reads the dock radius from round.conf instead of a hardcoded 14; dock 32 10 looks right.
- Tooltips: Sweet-mars widgets/tooltip.svgz, 9 body fills (5 edges and 4 corners, fill 1a1e21 fill-opacity 1) set to 0, force_glass word tooltip, round.conf tooltips 1. Confirmed by screenshot.
- Decorated frames: the square shadow outline around rounded corners is gone (checked over white). Real shadow margins of frames are 10 px per side.

### Lessons
- The KWin QPA Window format is never set, so alphaBuf -1 of internal windows means nothing; the backing store is always ARGB8888.
- Count the real matches before patching theme files (the tooltip svg had 9 fills, not 5).
- A running kwin keeps the first loaded plugin copy: install first, then log out and in.
- KWin scripts log with console.warn; window geometry properties are KWin::RectF. Check names in the source first.
- A temporary green shader shows the real glass footprint; sample screenshot pixels instead of guessing.
- Theme edits live outside the repo (Kvantum svg, Sweet-mars tooltip.svgz, backups .pre-menuglass and .pre-tooltipglass) and are not committed because they are third-party files.

### Not verified yet
- The per-window inner rect cache that should hide the menu outline during the fade-out.
- A 1 px gray line along the top and left of window frames that stops at the arc.

### Open items
1. Window button decoration idea (ripple or glow, user has not specified it yet).
2. Auto tier by GPU at login.
3. Launcher shader precision fix for NVIDIA (parked, NVIDIA not tested for now).
4. Animated context menus, GTK and Brave menus, notifications, wobbly plus glass.

## Session log 2026-11-08 to 2026-11-09 (Brave menus)

### Verified (log line or screenshot)
- Brave Origin (/opt/brave-origin-bin/brave) menus are popups of class brave with popupWindow true and type -1; the buffer is 24 px larger than the frame on every side, which points to native Wayland popups. Glass was already forced behind them by the menu word; the body was opaque.
- Menu body color is flat (34,34,35); text is (227,227,228); separators (42,42,43). Brave draws a dark-theme menu with subpixel text.
- Color key in the round mask shader: round.conf row key CLASS R G B RANGE MINALPHA. Pixels near the key color become see-through down to MINALPHA, applied to popups (kind 2) of that class only, on the un-premultiplied color so it also works while a menu fades in. Log line: round: key applied to brave. Edit round.conf and run reconfigureEffect liquidglassround to tune live.
- MINALPHA: 0.10 was too bright over white pages (light text on light glass, about 1.1:1), 0.55 looked too heavy against the rest of the OS; the user settled on the value from the 0.25 trial (the live value is in install/round.conf.example).

### Not verified
- The gray flash when a menu opens is not confirmed gone.
- glass off with Brave open: only the nested regression unloaded the round effect, never against live Brave windows; close Brave first.
- Subpixel text fringes (orange and blue) over bright glass; the user would like the text to matter more (brighter text with a faint dark halo is an idea, not built).

### Ideas
- Adaptive body alpha from the backdrop brightness; several key rows so other classes (GTK, other Chromium apps) can use the color key.
