# switchfish

Insaniquarium Deluxe as a Switch homebrew NRO. Read `README.md`, then `docs/ANALYSIS.md` for why things are
the way they are and `docs/ROADMAP.md` for what's next.

## Rules
- Build through `./sf.sh` (`build switch` runs in Docker, `build` is the host build). Both must keep building.
- Iterate on the host build; the Switch build is for validation. Platform-specific code goes in
  `framework/platform/switch/` or `framework/platform/default/`, guarded by `__SWITCH__` only where it must be.
- Our own code goes in `port/` (entry point, notice screens); `game/` and `framework/` stay close to upstream.
- `game/` is decompiled code: keep the original behaviour unless a change is a deliberate fix or an opt-in
  feature (widescreen, controls). Fixes from other decomps go in per file, with a note of where they came from.
- Never commit or bundle game assets. `gameData/` and `upstream/` are gitignored.
- `upstream/` is read-only reference. Don't run scripts from it on the host; build experiments go in Docker
  or /tmp.
- The Mac's filesystem is case-insensitive and the Switch SD card path handling is case-sensitive: never rely
  on case-insensitive file names, the framework goes through `fcaseopen`.
- Emulator: `./sf.sh emu` runs the NRO in Ryubing (portable, `tools/ryubing/`). Smoke tests only, real
  validation is on hardware.
- The widescreen tank sides (`port/romfs/widescreen/`) are drawn by an image model and only hold what it
  drew: the original's middle is cut out and the game pastes it back from the player's files. Never commit
  a full widened backdrop or anything else with PopCap pixels in it.
- Test runs: plans in `tests/plans/`, always short with `--timeout`; kill the game if it stalls. `--plan`
  runs muted, in the background, without vsync, and exits on its own 5 s past `--timeout` even if the
  main loop hangs; keep it that way. Run builds and test runs as separate commands, test runs with a
  short outer limit.
- No PopCap art in the repo or in a build made without the player's files. The NRO icon is cut from the
  player's own title screen at build time (`scripts/icon.py`, into `build/`); `switch/icon.jpg` is the fallback.
  The one deliberate exception: that fallback is the game's guppy redrawn by an image model (the owner's
  choice, 2026-11-22). Nothing else. "PopCap" may not appear in the product name.
- Keep the PopCap acknowledgment in README and in any release notes.

## Useful references in upstream/
- `insaniquarium_nx/platform/INTEGRATION.md`: every input conflict on Switch and how it was resolved.
- `OpenInsaniquarium/docs/re/`: timing (28 ms ticks), RNG, demo file format, asset formats, MO3 music remap.
- `Insaniquarium-Decompilation/`: most accurate decomp, Windows only, known regressions listed in
  `docs/research/decomp-comparison.md`.
