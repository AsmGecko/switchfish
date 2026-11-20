# Existing ports and frameworks (researched 2026-11-20)

Ranked by usefulness to us. All repos are cloned in `upstream/` (see `docs/upstream.txt`).

## Insaniquarium

### ChanseyIsTheBest/insaniquarium_nx: a working Switch port
- https://github.com/ChanseyIsTheBest/insaniquarium_nx, v1.0.0 (2026-09-05), v1.0.1 (2026-09-06). No LICENSE file.
- WinFish (vindirect) + kyle-sylvestre's PvZ-Portable `zuma` branch, both cloned unpinned by `setup.sh`.
  `port/port.sh` regenerates the game tree on every build with Python/sed fixups (`apply-framework.py`,
  `apply-portability.py`, `apply-cxx-modern.py`, `apply-gamefixes.py`).
- Fragile as a base (patches against moving upstream branches), but a complete catalogue of every WinFish
  portability problem. Read `platform/INTEGRATION.md` first.
- Input we want to learn from (`platform/Input.cpp`, `platform/nx_pointer.c`):
  left stick cursor, gyro pointing (`-`), cursor toggle (`+`), L/R recenter, A/ZL/ZR left click, **B right
  click** (the game checks for right click in Board, Coin, Fish, FishTypePet, Breeder, Grubber, Penta),
  X = Escape, Y = auto collect, stick click = Enter, D-pad sensitivity, touch in handheld, USB mouse. Cursor
  drawn just before `eglSwapBuffers`, synthesized MouseMove every frame so hover works.
- Presentation: fixed 1280x720 EGL surface, 4:3 pillarbox at 960x720, hardware upscales when docked.
- Saves byte-compatible with PC in `userdata/`.

### kyle-sylvestre: Insaniquarium Portable (our base)
- https://github.com/kyle-sylvestre/WinFish `develop` (84 commits on top of vindirect's 2026-03-13 state,
  last 2026-10-31) and https://github.com/kyle-sylvestre/PvZ-Portable `insaniquarium` (last 2026-10-30).
- Builds for Windows, macOS, Linux, Android (itch.io v0.8) and Switch (opt-in CI job, never run in CI).
- Framework: SDL2 + GLES2 (`graphics/GLInterface.cpp`, `GLImage.cpp`), SDL-Mixer-X + libopenmpt for MO3,
  file-backed registry (`misc/RegEmu.cpp`), `fcaseopen` for case-insensitive paths,
  `platform/{default,switch}/{Window,Input}.cpp`.
- Game side: about 60 decomp bugfix commits, save and mute on focus loss, screensaver and update check off.
- Switch input in the framework is minimal (touch + buttons, no stick cursor).
- License: framework LGPL-3.0-or-later plus PopCap Framework License; WinFish "as-is, modifications MIT".
- We verified it builds to an NRO in our Docker image (only an mpg123 link-order fix needed).

### Other Insaniquarium projects
| Repo | What | License | Use |
|---|---|---|---|
| vindirect/winfish | the original C++ decomp, Win32/DirectX/BASS, fixes until 2026-10-08 | as-is / MIT mods | reference |
| 5N00Z3R/Insaniquarium-Decompilation | one-shot re-decompilation (2026-10-03), more accurate against the binary | none | accuracy reference, see `decomp-comparison.md` |
| elgreams/OpenInsaniquarium | Rust/Bevy clean-room rewrite, tick-verified, tank 1 only | MIT/Apache | RE docs in `docs/re/`, demo format, timing, RNG |
| SaMeiers/insaniquarium-port | WinFish on PopLib (SDL3), PortMaster | AGPL-3.0 | reference only |
| Dangerous-J-Dev/insaniquarium-mac | WinFish on kyle's framework, macOS | MIT | reference only |
| KartingSackboy06/insaniquarium-vita | so-loader around kyle's Android build | none | control ideas (Presto menu, right click) |

## SexyAppFramework ports
| Repo | Notes |
|---|---|
| wszqkzqk/PvZ-Portable | upstream of kyle's framework. LGPL-3.0+, very active, moved to SDL3 on 2026-11-08 (#523). Switch code still exists but CI job is commented out, SDL3 Switch needs an unofficial patch. Useful issues: #196/#359 (`std::thread::detach()` throws on libnx), #148 (debug with unstripped ELF). |
| headshot2017/re-plants-vs-zombies | CC0 origin of the Switch GL/input layer (`SexyAppFramework/platform/switch`). Escape hatch if we ever want to leave LGPL. |
| Puyodead1/bejeweled-deluxe-nx | keeps software DDImage rendering and uploads one texture per frame via SDL_Renderer. Minimal-change approach, good Eden emulator scripts. |
| Patoke/re-plants-vs-zombies | still Win32. |

Not applicable: ChanseyIsTheBest's Zuma/Peggle/Bejeweled 3 NX ports (Xbox 360 static recompilation),
aks796/pvzdlc_nx (x86 to C translation), PopSilicon (Mac binary loader).

## Gotchas collected from these projects
- `typedef unsigned long ulong` is 8 bytes on aarch64: pixel buffers, saves, pak parsing.
- `std::thread::detach()` throws on devkitA64/libnx: keep loading threads joinable.
- `switch.h` breaks `EGLAPIENTRYP`: define it before `<EGL/egl.h>`.
- SD card paths are case-sensitive through fsdev: use fcaseopen.
- `__nx_stack_size` = 4 MB.
- Album (applet) mode has little memory and no swkbd: tell users to use title takeover (hold R on a game).
