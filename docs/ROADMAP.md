# Roadmap

Each milestone has an exit criterion; don't start the next one until it holds.

**Next up** (from `docs/STATUS.md`): the crash on quit, tuning the controls on hardware (M3), determinism
check and robust test plans (M2), decomp fixes (M4). Released: 0.1.0 (2026-11-22).

## M0: Repo and toolchain (done 2026-11-20)
- [x] Reference repos in `upstream/`, research in `docs/research/`
- [x] Docker image with devkitA64 r30, libnx 4.12, SDL2, mesa, libopenmpt 0.8.10
- [x] Game + framework vendored, CMake for host and Switch, `./sf.sh`
- [x] `switchfish.nro` and a macOS host binary build

**Exit:** both builds succeed from a clean checkout.

## M1: First light
- [x] Host build reaches the main menu with real game files in `gameData/` (2026-11-20)
- [x] NRO boots on a Switch (title takeover) with the files in `sdmc:/switch/switchfish/` (2026-11-21)
- [x] NRO boots in Ryubing with the game files and reaches tank 1-1 (`./sf.sh emu`, 2026-11-20)
- [x] Notice screens in the style of the game's dialogs: game files missing, started from the album
      (`port/Notice.cpp`, preview with `--notice album|nodata`)

**Exit:** tank 1-1 playable on hardware with touch.

## M2: Test harness
- [x] Plans with clicks, keys and screenshots per logic tick, time limit, forced focus (`port/TestPlan.cpp`)
- [x] `--quickplay` (N finishes a level) and `--level T-L`
- [ ] Waits on named probes (screen, money, level) and asserts, so plans stop depending on timing
- [ ] Determinism check: per-tick state hash, classic 4:3 identical to the pre-widescreen build
- [ ] Headless host runs (SDL dummy video, uncapped loop), `./sf.sh test`
- [ ] Re-attach `.dmo` recording to the SDL event path; replay as regression tests
- [ ] Plans for: new profile to tank 1-1 won, a full tank 1, store/pets, virtual tank
- [ ] Same harness in the NRO via `nxlink --args`

**Exit:** `./sf.sh test` plays tank 1 hands-free and fails on a crash or a stuck level.

## M3: Switch controls and polish
- [x] Analog stick cursor, click mapping incl. B = right click, cursor speed modifiers (`docs/CONTROLS.md`)
- [x] Direct touch with our own coordinate mapping (SDL touch-to-mouse off)
- [x] swkbd for player names (current text and length)
- [x] D-pad to buttons, USB mouse, pause on focus loss
- [x] Rumble on aliens, hits, glass taps, deaths
- [ ] Tune on hardware; gyro, single Joy-Con
- [ ] Docked/handheld switching, audio restart after resume
- [x] 4:3 presentation with sharp-bilinear scaling; the help screen's water beside 4:3 screens
- [x] Quick Aim (X/Y), fast-forward (R), a Controls page in Help, rumble and Quick Aim settings
- [ ] Crash on quit on hardware (hbloader 2001-0106)
- [ ] Remove the framework's dependency on game resources

**Exit:** a full adventure tank plays comfortably docked (Pro Controller) and handheld (touch).

## M4: Code health
- [ ] Merge vindirect's fixes after `61ddba1` (32 commits)
- [ ] Pull in Insaniquarium-Decompilation's accuracy work per file (skip its known regressions, see
      `docs/research/decomp-comparison.md`), each change checked with the harness
- [ ] Drop dead Windows-only code (BetaSupport, InternetManager, registration)
- [ ] Warnings clean on GCC 16 and clang

**Exit:** harness plans for all modes pass on host and Switch.

## M5: Widescreen (see docs/WIDESCREEN.md "Status")
- [x] Framework: configurable logical canvas (640x480 or 854x480), viewport and input remapping
- [x] Widescreen presentation: extended backdrops around the user's own art, HUD centered; title screen
      and main menu widened, Hall of Fame, Help, stories and credits across the screen
- [x] True widescreen tank: tank width constant replaces the hard-coded bounds
- [ ] Setting in the options dialog (done); 4:3 bit-identical to the original (harness check still open)

**Exit:** all modes play in 16:9 on the full Switch screen, 4:3 mode passes the same plans as before.

## Later
- SDL3 once devkitPro packages it
- Gyro pointing
- Test plans in CI (CI builds the NRO per push and attaches it to releases since 2026-11-22)
