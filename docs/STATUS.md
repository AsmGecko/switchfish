# Status: how solid is this (2026-11-20)

As a foundation it is good. Stable it is not yet: on hardware, in the controls and in the tests most
things are assumed rather than shown.

## What is solid

- **The base is proven.** A real source port, no patch scripts. Runs on the Mac, boots in the emulator,
  both builds come from one repo with one command (`sf.sh`) and a pinned Docker toolchain.
- **Provenance and licences are clear.** Every vendored folder is listed with commit and licence in
  `docs/UPSTREAM.md`. No PopCap art in the repo.
- **Widescreen is done the clean way.** PopCap's own widescreen convention, still in the framework, so
  menus, HUD and dialogs are untouched. The classic 4:3 tank is designed to keep every original number.
- **Hands-off testing has a start.** Test plans click through the game and take screenshots, and
  `--quickplay` / `--level` make it quick to reach any level.

## Where it is shaky (most risky first)

| # | Risk | Why |
|---|---|---|
| 1 | **Never run on a real Switch** | Only a short check in Ryubing. Performance, memory, the mesa driver, the new canvas and the controls are unknown. Biggest risk. |
| 2 | **Controls untested on hardware** | Stick cursor, touch, B, D-pad, menu/pause, USB mouse are built (`docs/CONTROLS.md`) and checked on the Mac and in Ryubing, but how they feel and focus/sleep handling need a real Switch. Gyro and rumble wait for that. |
| 3 | **"4:3 is identical" is claimed, not proven** | The ~95 converted bounds were checked by me, an agent and Codex, but no test checks it. Exactly the kind of mistake that slips in silently. |
| 4 | **The decomp itself** | We build on kyle-sylvestre's fork. It misses 32 later fixes from vindirect and the more accurate re-decompilation by 5N00Z3R. There are bugs we don't know about. |
| 5 | **Test plans are brittle** | Fixed ticks and click positions. A dialog that wasn't there before makes the rest click beside the target (happened several times). No "wait until this screen" steps, no values read from the game (money, level). |
| 6 | **The widescreen sides** | In the repo and the NRO now, but regenerating them needs Codex (paid, not reproducible). Only tanks 2 and 3 seen in the game itself. |
| 7 | **Known loose ends** | The HUD bar floats. Screens like pets and instructions still show the old tank at the sides. Pet and food dialogs are 640 wide. Once the game hung on exit (not seen again). |
| 8 | **Maintenance** | The LGPL framework is changed a lot, so taking upstream fixes is manual. No remote, no CI. The Docker image pulls the newest packages when built, so it isn't fully reproducible. |

## What to do first to make it stable

1. **Test on real hardware** (Atmosphère, title takeover). Removes the biggest unknown.
2. **Switch controls**: built (stick cursor, touch, B, D-pad, mouse); tune on hardware, then gyro and rumble.
3. **Determinism check**: the same input in 4:3 must give the same state tick for tick as before the
   widescreen changes. Plus "wait until" steps and game values in test plans so they stop breaking on
   timing.
4. **Remote on Gitea plus CI** that builds the NRO and runs the test plans on every push.
5. **Take in the decomp fixes**, per file, checked with those test plans.

The architecture and the choices are right; what's missing is proof.
