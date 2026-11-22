# switchfish: analysis and approach

Status 2026-11-20. Goal: Insaniquarium Deluxe on Nintendo Switch, eventually widescreen across the whole
screen, in a clean and stable repo. Research notes behind every claim are in `docs/research/`.

## 1. Starting point

Three projects were asked for, and research turned up more:

| Project | What it is | Role for us |
|---|---|---|
| vindirect/winfish | C++ decompilation of Insaniquarium Deluxe 1.1 on PopCap's SexyAppFramework (Win32, DirectDraw/D3D7, BASS) | origin of the game code |
| 5N00Z3R/Insaniquarium-Decompilation | a more accurate re-decompilation (2026-10-03), Windows only | accuracy reference |
| elgreams/OpenInsaniquarium | Rust/Bevy clean-room rewrite, tick-verified against the original, tank 1 only | reverse engineering docs, test oracle |
| kyle-sylvestre WinFish + PvZ-Portable | WinFish on an SDL2/GLES2 SexyAppFramework, runs on Windows/macOS/Linux/Android/Switch | **our base** |
| ChanseyIsTheBest/insaniquarium_nx | a released Switch NRO (v1.0.1, 2026-09) built from patch scripts | reference for controls and fixes |

**An Insaniquarium Switch port already exists** (insaniquarium_nx). It proves the route works. It is built by
regenerating the source on every build from Python/sed patches against unpinned upstream branches, has no
license, and stops at a pillarboxed 4:3 image. That is not the stable repo we want, but it is the best
catalogue of problems and control solutions there is.

## 2. Options considered

| Option | Verdict |
|---|---|
| Fork insaniquarium_nx | fragile (patches against moving branches), no license, no real source history. Use as reference. |
| Port winfish ourselves (replace DirectX/BASS/Win32) | months of work kyle and PvZ-Portable already did. |
| Software rendering + one texture upload (Bejeweled NX style) | least framework change, but we would still write all the platform code, and widescreen/effects are harder. |
| OpenInsaniquarium on Switch | Rust on Switch is no_std tier 3 with no GPU stack; Bevy cannot run there. Also only tank 1. |
| **kyle's WinFish + SDL2/GLES2 framework, vendored into one repo** | **chosen.** Builds for Switch today (verified), runs on desktop for fast iteration, real source we can change. |

## 3. Toolchain decision

devkitPro (devkitA64 r30 / gcc 16.1, libnx 4.12) is the only maintained Switch homebrew toolchain; every
serious 2026 project uses it. Details and dates in `docs/research/toolchain.md`.

| Layer | Choice | Why |
|---|---|---|
| Toolchain | devkitA64 + libnx in Docker (`docker/Dockerfile`, pinned base `devkita64:20260219`) | native arm64 image, reproducible, nothing installed on the Mac |
| Window/input/audio | SDL2 2.28.5 (`switch-sdl2`) | only packaged SDL; SDL3 exists as a devkitPro branch but is not packaged. Move later. |
| GPU | GLES2 on mesa/nouveau (EGL) | what SDL2 and every reference port uses; mesa 20.1 is old but frozen and working. deko3d is the escape hatch. |
| Music | libopenmpt 0.8.10, built in our image | game music is MO3 (`music/*.mo3`); nothing packaged plays MO3 |
| SFX | SDL-Mixer-X (vendored) | already wired into the framework |
| Host build | same CMake, Homebrew SDL2/libopenmpt | iterate on the Mac, Switch build for validation |

Dated pieces (mesa 2020, SDL2 2.28) are accepted deliberately: they are what the whole Switch homebrew
scene runs on, and our platform layer stays thin enough to swap.

## 4. What we verified today
- `./sf.sh build switch` produces `build/switch/switchfish.nro` (11 MB) from this repo in Docker.
- `./sf.sh build host` produces a native macOS build.
- libopenmpt cross-compiles and links for Switch.
- Not yet verified: running it. That needs your own game files and a Switch (or an emulator smoke test).

## 5. Repo layout and provenance
```
game/          WinFish game code       from kyle-sylvestre/WinFish develop @97cfa39aeb
framework/     SexyAppFramework SDL2   from kyle-sylvestre/PvZ-Portable insaniquarium @11fe5bc67a
third_party/   SDL-Mixer-X             vendored by that framework
docker/        Switch build image
switch/        NRO icon (the guppy redrawn by an image model)
docs/          this analysis, roadmap, research
upstream/      reference clones (gitignored, ./sf.sh upstream)
gameData/      your own Insaniquarium files for host runs (gitignored)
```
Changes made while importing: framework includes in the game rewritten to real paths (no symlink "flat"
directory), PvZ `Resources.h` include in five framework widgets pointed at the game's `Res.h`, entry point
rewritten (`game/Main.cpp`), Windows/Android/3DS/Emscripten platform code dropped, CMake trimmed to host and
Switch. Licensing: see `docs/UPSTREAM.md`.

## 6. Controls (design, from the reference ports)
| Input | Action |
|---|---|
| Touch (handheld) | direct: pointer jumps to finger, tap = left click |
| Left stick | analog cursor, time-based with sub-pixel accumulator and radial deadzone |
| ZR (hold) / ZL (hold) | faster / slower cursor |
| A | left click (held = repeat, the game wants rapid clicking) |
| B | right click (game uses it in 7 places: selling fish, feeding pets) |
| Plus | pause / options |
| X | escape / back |
| Y | auto collect (a game option) |
| USB mouse | works in both modes |

Gyro pointing (insaniquarium_nx) is a nice optional extra. Text entry (player name) via swkbd.

## 7. Widescreen
The game is built around a 640x480 tank. Measured in `game/`: about 185 hard-coded 640/480-style
coordinates in 27 files (tank bounds such as `mXMax = 540`, HUD bars, full-screen fills), and every tank
backdrop is a 640x480 image. Three levels, in order:

1. **4:3, done well.** 960x720 centered, sharp-bilinear scaling, side bars with a blurred/darkened copy of
   the current backdrop instead of black. Cheap, no gameplay change. This is where insaniquarium_nx stops.
2. **Widescreen presentation.** Logical canvas 854x480 (16:9). Tank screens draw the backdrop extended
   sideways (generated at load time from the user's own image: mirrored and blended edges, so we ship no
   art), HUD centered. Gameplay area stays 640 wide. Menus stay 4:3 centered.
3. **True widescreen tank.** Replace the hard-coded bounds by a tank-width constant so fish, food, coins and
   aliens use the full 854 width. Changes balance slightly, so it is an option. With the 4:3 setting the
   game must stay exactly the original, which the test harness (section 8) checks.

The framework already renders through GL with a viewport, so a wider logical canvas is a framework change
(SexyAppBase width/height, GLInterface viewport, input remapping), not a rewrite.

## 8. Hands-off testing
No Switch emulator has scripting like Felk's Dolphin (no Python/Lua, no memory API; Eden has TAS files for
buttons only, GUI-triggered). Details in `docs/research/testing-automation.md`. Because we own the source,
we build the harness into the game instead:
- JSON test plans: click/key at tick N, wait until a named probe (money, level, fish count) reaches a value,
  screenshots, asserts. Input enters at the Sexy event level, so the same plan works with touch or stick.
- Host: headless (SDL dummy video), uncapped speed, so a whole level runs in seconds. Like devilutionX's
  timedemo test.
- The framework's `.dmo` demo record/playback (still in the code) for replaying real play sessions.
- Switch: the same harness in the NRO via `nxlink --args`, plus sys-botbase on real hardware for touch/HID
  input and screenshots, Atmosphère gdbstub for crashes.

## 9. Known risks
| Risk | Mitigation |
|---|---|
| kyle's framework is LGPL-3.0+ | fine for an open source NRO; headshot2017's CC0 layer is the exit if ever needed |
| framework depends on game resources (`FONT_PICO129`) | cleanup item on the roadmap |
| game code is a decomp with remaining bugs | B's accuracy pass and A's later fixes, merged per file with harness checks |
| aarch64 differences (float to int, unsigned char, aliasing) | `-fsigned-char -fno-strict-aliasing`, harness diffs |
| applet (album) mode has little memory | detect it and tell the user to use title takeover |
| no hardware tested yet | first milestone |
