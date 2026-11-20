# winfish vs Insaniquarium-Decompilation (compared 2026-11-20)

- **A**: vindirect/winfish, 38 commits Jan to Oct 2026.
- **B**: 5N00Z3R/Insaniquarium-Decompilation, single dump 2026-10-03.
- **K**: kyle-sylvestre/WinFish `develop`, forked from A at `61ddba1` (2026-03-13), 84 commits of its own,
  32 commits of A behind. This is what `game/` is today.

## Lineage
B was re-decompiled rather than edited: its WinFish diff against every A commit is about 29.7k lines. It has
every A fix up to 2026-10-03 and equivalents of A's later fix #4 to #9 (`FishSongMgr.cpp:58`,
`DeadFish.cpp:243`, `BonusScreen.cpp:170`, `OtherTypePet.cpp:743`, `Board.cpp:5494`, `FishTypePet.cpp:363`).
A's `Beetlemuncher.cpp` and `ColorUtils.*` are dead code.

## What B improves
An accuracy pass against the binary: 462 "original" comments and 110 addresses (A: 1 each). Control flow in
the original's order (Fish::Update Wadsworth/Gumbo, Coin::Update removal), RNG draw order matched, float vs
double truncation matched, `HighScoreMgr::GetPerLevelEntry` rewritten, ambiguous `ButtonListener` bases
removed, XMLParser stale iterators fixed.

## B regressions to avoid if we pull files from it
- `Common.cpp:505,518` `std::locale("English_United States")` terminates on non-Windows.
- `Board.cpp:934` loop of 10 over `float mTankLightingSpeeds[3]` overwrites `mMoney` (use 3).
- `Board.cpp:834,990,1094,5639` uninitialised locals (834 is a `%` divisor).
- `DataSync.cpp:181` `delete` on `new[]` memory.
- `DescParser.cpp:504` erase on empty string (A's fix #1), likely crashes on libstdc++.
- New Windows-only code: SexyCache (shared memory IPC), BigInt/md5 reg check, InternetManager, BetaSupport.

## Portability hazards (all trees)
1. `ulong` is 8 bytes on aarch64: pixel buffers, `DataSync` ReadLong/WriteLong, pak header.
2. `Res.cpp:1120,1122` pointers truncated to int as map keys.
3. `(int&)` punning of 64-bit save fields (`GameObject.cpp`, `ProfileMgr.cpp`): build with `-fno-strict-aliasing`.
4. `char` is unsigned on ARM: `-fsigned-char`.
5. Out-of-range double to int saturates on ARM instead of giving 0x80000000.

K already fixes most of the compile-level problems for GCC/clang (it builds for Switch and Android).

## Decision
Base on K because it builds and runs on the SDL framework today. Pull B's accuracy work in per file, verified
with replays/harness, as a tracked roadmap item. Merge A's fixes since `61ddba1` the same way.
