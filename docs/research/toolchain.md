# Switch toolchain: is devkitPro the best tool? (researched 2026-11-20)

Short answer: yes. devkitA64 + libnx is the only maintained Switch homebrew toolchain, and SDL2 on mesa
GLES2 is the best trodden path for a 640x480 2D game. Some packages are old; we build those ourselves.

## Toolchain
- libnx: 4.9.0 (2025-05), 4.10.0 (2025-11), 4.11.0/4.11.1 (2026-02), 4.12.0 (package 2026-02, GitHub release
  2026-09-28). About two releases a year, commits continue (2026-10).
- devkitA64 r30 (gcc 16.1.0, binutils 2.46) released 2026-06-10. Our image gets it via `dkp-pacman -Syu`.
- Docker `devkitpro/devkita64:20260219` is multi-arch, runs natively on Apple Silicon.
  devkitPro asks people to use the Docker images in CI rather than pacman.
- Atmosphère is active: 1.12.0 (2026-09-27), 1.12.1 (2026-10-16), 1.12.2 (2026-11-11).

Alternatives checked:
- libtransistor: archived 2019.
- Rust: `aarch64-nintendo-switch-freestanding` is tier 3, no_std, no GPU stack. Rules out OpenInsaniquarium
  (Bevy) on Switch.
- Zig: only toy repos.
- Nintendo SDK: NDA, not an option.
- Every 2026 Switch recompilation project (SwitchWakerHD, nsmbw-switch, ...) also builds on devkitPro.

Switch 2 does not run Switch 1 homebrew without its own CFW, and there is no persistent CFW (tethered
jailbreak in Oct 2026, patched in 23.5.0). Target is a hacked Switch 1.

## Graphics
| Option | State | Verdict |
|---|---|---|
| mesa/nouveau GL/GLES via EGL (`switch-mesa` 20.1.0) | based on mesa 20.1.0-rc3 (May 2020), frozen but working. Runtime GLSL compile, about 6.7 MB NRO overhead | **use it**, what SDL2 and all reference ports use |
| deko3d | native low-level API, maintained (commits 2026-10), offline shaders via uam | escape hatch if we ever need the performance, more work |
| SDL2 renderer | GLES2 on mesa, no deko3d renderer | fine, but the framework does GLES2 itself |
| Vulkan | not available in homebrew | |

## SDL
- `switch-sdl2` 2.28.5-4 (built 2024-03), branch `switch-sdl-2.28` still gets fixes (2026-02).
- SDL3: devkitPro has `switch-sdl-3.2` and `switch-sdl-3.4` branches (Feb 2026 work by WinterMute), builds
  cleanly, but no package yet (pacman-packages#432 open). Upstream libsdl will not take a homebrew Switch
  backend (#6822).
- Decision: SDL2 now. Keep platform code thin so SDL3 is a mechanical move once devkitPro packages it.

## Audio
- SDL2 audio uses audren (48 kHz native).
- `switch-sdl2_mixer` 2.0.4 is from 2018 and cannot play MO3. Our framework vendors SDL-Mixer-X instead.
- libopenmpt is not packaged: we build 0.8.10 in `docker/Dockerfile` (with mpg123 + vorbis for MO3 samples).

## What is dated and how we deal with it
| Piece | Version | Mitigation |
|---|---|---|
| devkitA64 | r30 via -Syu | pin base image tag |
| switch-sdl2 | 2.28.5 | fine; build branch head if audio issues show up |
| switch-mesa | 20.1 (2020) | frozen and working; deko3d if ever needed |
| SDL2_mixer | 2.0.4 | not used, SDL-Mixer-X vendored |
| libopenmpt | not packaged | built in our image |
| SDL3 | branch only | migration target later |
