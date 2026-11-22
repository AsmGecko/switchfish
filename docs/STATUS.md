# Status: how solid is this (2026-11-22)

As a foundation it is good, and it plays on a real Switch (0.1.0 is released). Stable it is not yet: the
crash on quit is open, and in the tests most things are assumed rather than shown.

## What is solid

- **The base is proven.** A real source port, no patch scripts. Plays on a Switch (title takeover from
  Sphaira), on the Mac and in Ryubing; both builds come from one repo with one command (`sf.sh`) and a
  pinned Docker toolchain, and CI builds the NRO on every push and attaches it to releases.
- **Loading is fast.** From 38 s to about 5 s on hardware, less with the image cache (`docs/LOADING.md`).
- **Provenance and licences are clear.** Every vendored folder is listed with commit and licence in
  `docs/UPSTREAM.md`. No PopCap art in the repo.
- **Widescreen is done the clean way.** PopCap's own widescreen convention, still in the framework, so
  menus, HUD and dialogs are untouched. The classic 4:3 tank is designed to keep every original number.
- **Hands-off testing has a start.** Test plans click through the game and take screenshots, and
  `--quickplay` / `--level` make it quick to reach any level.

## Where it is shaky (most risky first)

| # | Risk | Why |
|---|---|---|
| 1 | **Crash on quit on hardware** | hbloader stops with 2001-0106 after the game has returned: memory it lent to a service or the GPU is still in use. The game saves first, so nothing is lost, and the latest build logs the blocks still held at exit. |
| 2 | **Controls only briefly played on hardware** | Stick cursor, touch, B, D-pad, Quick Aim, fast-forward and USB mouse are built (`docs/CONTROLS.md`) and played a little on a Switch; rumble strengths are a guess, gyro waits. Much was only checked on the Mac and in Ryubing. |
| 3 | **"4:3 is identical" is claimed, not proven** | The ~95 converted bounds were checked by me, an agent and Codex, but no test checks it. Exactly the kind of mistake that slips in silently. |
| 4 | **The decomp itself** | We build on kyle-sylvestre's fork. It misses 32 later fixes from vindirect and the more accurate re-decompilation by 5N00Z3R. There are bugs we don't know about. |
| 5 | **Test plans are brittle** | Fixed ticks and click positions. A dialog that wasn't there before makes the rest click beside the target (happened several times). No "wait until this screen" steps, no values read from the game (money, level). |
| 6 | **The widescreen sides** | Tanks, title screen and main menu art is in the repo and the NRO, but regenerating it needs Codex (paid, not reproducible). |
| 7 | **Known loose ends** | The HUD bar floats. Pet and food dialogs are 640 wide. Framed 4:3 screens (store, pets) sit on water rather than wide art. |
| 8 | **Maintenance** | The LGPL framework is changed a lot, so taking upstream fixes is manual. CI builds but runs no tests yet. The Docker image pulls the newest packages when built, so it isn't fully reproducible. |

## What to do first to make it stable

1. **Fix the crash on quit** with the exit log from hardware.
2. **Switch controls**: built; play and tune on hardware, then gyro.
3. **Determinism check**: the same input in 4:3 must give the same state tick for tick as before the
   widescreen changes. Plus "wait until" steps and game values in test plans so they stop breaking on
   timing.
4. **Test plans in CI**: it builds the NRO on every push now; running plans needs a headless host build.
5. **Take in the decomp fixes**, per file, checked with those test plans.

The architecture and the choices are right; what's missing is proof.
