# Quality of life and new features

From a research pass over the code (2026-11-21) plus the player's own ideas after playing on hardware. Each
idea is **faithful** (no change to play or balance) or an **assist/new** feature (changes play, so opt-in
and off in the classic game). Free buttons: X, Y, L, R and both stick clicks.

## Work list

Loading on the Switch comes first: `docs/LOADING.md`.

- [x] 1. Save when the game is closed from Home (needs a check on hardware)
- [x] 2. B and + close dialogs (Escape on a keyboard too)
- [x] 3. X jumps to the hungriest fish, Y to money ("Quick Aim (X/Y)" in Options, on by default)
- [x] 3b. Remove the Hardware Acceleration checkbox (Is3DAccelerated is always true here)
- [x] 3c. Write saves and settings atomically (temp file + rename, .bak on the Switch)
- [x] 4. Controls page in the Help screen (page 9), a Controls button in Options
- [ ] 5. Controls & Extras dialog: rumble on/off, the jump buttons, fast-forward, cursor auto-hide
- [ ] 6. Volume sliders on the D-pad
- [ ] 7. Hide the cursor when idle; Virtual Tank aquarium view
- [ ] 8. Fast-forward (R, opt-in)
- [ ] 9. Marker over hungry fish
- [ ] 10. Statistics page
- [ ] 11. Two-cursor co-op (last, opt-in)

## The ideas

### 1. Save when the game is closed from Home (faithful, S)
When `appletMainLoop()` fails, the Switch input code sets `mShutdown` directly
(`framework/platform/switch/Input.cpp`), after which `WinFishApp::Shutdown` returns at once, so
`RemoveBoard()`/`SaveCurrentGame()` never run: the level in progress is lost on Home → Close or when
another game is started under title takeover. `appletLockExit()` at start, and on the exit request run the
real `Shutdown()` (which saves) before unlocking. Optionally save on focus loss when `mShouldSave` is set.

### 2. B closes dialogs (faithful, S-M)
Nothing in the game handles Escape except the board, so + and B do nothing in Options, Pause, Quit and the
rest. While a modal dialog is open, B sends Escape instead of a right click; `Dialog::KeyDown` presses the
No/Cancel button, or the only button. Check every dialog id (`WinFishApp.h`) so B never confirms something
that can't be undone.

### 3. Y = hungriest fish, X = money (input aid, M)
- Hunger is `GameObject::mHunger`, counting down per tick; only food eaters (`mFishList`, `mBreederList`)
  and only fish whose hunger the game already shows (`IsHungryVisible()`, or Blip's pointer range when Blip
  is in the tank). Most urgent (lowest `mHunger`) first. Clicking a guppy already drops food there.
- Money: `mCoinList` plus `mNikoPearlCoinList`, skipping collected coins, notes and fresh beetles. Order:
  shrapnel bombs first (they kill fish), then coins on the floor by time left, then falling coins by value.
- Pressing again within about 1.5 s without moving the stick cycles to the next candidate. The cursor
  doesn't follow, holding doesn't sweep, A still clicks; aliens are never a target (knockback depends on the
  click point). No candidates: nothing (a tiny "no" rumble).
- Built in `PadCursor` next to the D-pad jumps, asking the game through a hook (like the rumble hook); the
  candidate code in `port/`. Off during dialogs, pause and bonus rounds. Behind a setting.

### 4. Controls page (faithful, M)
`HelpScreen` draws pages through a switch with a title and four rows, each with an animated icon box ("The
Basics" is the template). A CONTROLS page with the game's own art and the button letters in its font: stick,
ZL/right stick, A/ZR, B, D-pad, X/Y, +/−, touch. Reachable from a Controls button in Options (the Check
Updates slot is free) and as page 1 of Help. The tank 1 instruction page and the pause dialog could carry
a short button reminder.

### 5. Controls & Extras dialog (faithful container, M)
A `MoneyDialog` with labelled checkboxes like `CheatsDialog`, opened from Options. Holds rumble on/off, the
jump buttons, fast-forward and cursor auto-hide; settings in the registry like Widescreen.

### 6. Volume sliders on the D-pad (faithful, S)
Sliders aren't collected by the D-pad jump; collect their thumbs and let left/right step them by 0.1. When
D-pad down finds nothing, go back to where the cursor was in the tank before it jumped to the HUD.

### 7. Hide the cursor when idle; Virtual Tank aquarium view (faithful, S-M)
Hide the pad cursor after about 3 s idle (system screenshots include it). In the Virtual Tank one button
(R3) hides the trophy bar, HUD and cursor, and `appletSetMediaPlaybackState(true)` keeps a docked Switch
awake: a living aquarium without the real screensaver mode (which changes save paths).

### 8. Fast-forward (new, opt-in, S)
The framework already has `mUpdateMultiplier`. Hold R for 2× while a board is active and not paused, reset
on menus. Logic stays tick exact; scores count ticks. Adventure and sandbox only (it would speed up shell
farming in the Virtual Tank). 0.5× slow motion as an accessibility option.

### 9. Visual aids (S-M)
A marker over visibly hungry fish (faithful: the same information as the green sprite), for colourblind
players and handheld. Blinking coins about to vanish would be new information: opt-in assist.

### 10. Statistics (faithful, M)
Counters for coins, aliens killed, fish lost, levels, in a separate per-profile file (`users.dat` must stay
compatible), shown as a help-style page or a dialog. Achievements could build on it later.

### 11. Two-cursor co-op (assist, L)
The game has one pointer (hover state, food and gun repeat read the last mouse position). A second Joy-Con
could send single clicks only; fun but hacky, opt-in, last.

### Not worth it
- Cloud save.
- A fish gallery: the pets screen, the Virtual Tank store and help pages already show them.
- Aim magnetism toward aliens: changes knockback.
- Auto-collect or turbo sweep: hands out the machine gun early.
- The real screensaver mode: changes save paths; item 7 gets the same result.
- Colourblind palette shaders: hunger is a sprite swap, a marker works better.
- A button remapping screen: too few buttons.
- Online leaderboards: no network.
- HD or redrawn art: PopCap art.
