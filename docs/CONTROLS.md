# Switch controls

The game is mouse only. On the Switch a pad cursor, touch and a USB mouse all drive the one pointer the
game knows. Designed from what the game actually does, compared against insaniquarium_nx
(see the end for what we took from it and what we did differently).

## What the game needs

- **Left click**, fast and often: food, coins, aliens. Holding it repeats food (always) and the gun (top
  weapon only), `Board.cpp:835-868`. Where you hit an alien decides its knockback (`Alien.cpp:564-600`),
  so the click point is never moved for the player.
- **Right click** taps the glass (`Board::MouseDown`, click count < 0 → `CheckMouseDown`): a sound, scatters fish in the
  Virtual Tank, opens Presto's menu. It doesn't sell or feed.
- **Keys**: space pauses (`Board::KeyChar`), `b` bubbles, cheat words, the Konami code on the selector.
  The original handles neither Escape nor the F-keys; we made Escape open the board's menu.
- **Hover**: buttons light up and the hand cursor shows over clickable things.
- **Text** for player names (swkbd).

## Button map

| Input | Action |
|---|---|
| Left stick | cursor |
| A, ZR | left click, holding is a real hold |
| B | right click (tap the glass) |
| ZL (hold) | precision, 0.35× speed |
| Right stick | slow nudge, 0.25× speed |
| + | menu (Escape; the board opens Options, like its Menu button) |
| − | pause (the game's own space key) |
| Touch (handheld) | direct pointing, one finger; hold ZL while tapping for a right click |
| D-pad | jump to the nearest button that way (plus arrow keys) |
| X | jump to the hungriest fish the game shows as hungry and click (drops food); again goes to the next fish |
| Y | jump to money and click: bombs first, then what lies on the floor longest, then what falls lowest (not in bonus rounds) |
| X/Y held | again every 0.3 s; with ZL held they only jump, without clicking |
| A twice | double click (within 0.4 s and 6 units) |
| USB mouse (dock) | moves the same cursor, left and right button click |

In the game, Options > Controls (Help page 9) explains this in the game's own style.

Home and Capture stay with the system. Losing focus (Home, sleep) pauses and mutes like alt-tab does.

## Cursor

- Radial deadzone 0.12, full at 0.95. With m the deflection past the deadzone (0..1), speed is
  20 + 1080·m² design units/s: 117 at 0.3, 290 at 0.5, 1100 at full (across the 640 tank in 0.58 s).
- Held above 0.9 for 0.25 s, the top speed ramps to 1600 over 0.3 s, for the wide tank.
- Time based (dt clamped to 50 ms), float position.
- Clamped to what can be clicked: the whole canvas on the wide tank, 0..640 elsewhere. Rechecked every
  frame, so the Widescreen option can change live.
- Drawn with the game's own cursor art (pointer, hand) after the canvas is presented, at presentation
  scale; never into the canvas (it keeps its contents between frames). Screenshots from test plans read
  the canvas, so they don't show it.
- Half speed while the game shows its hand cursor (over buttons, coins, some pets), so it is easy to stop on
  a coin. ZL precision overrides it.
- Hidden when touch takes over; the stick continues from where the finger was. + and − don't show it.

## Touch

Follows one finger (the first down) and ignores the rest. A press that starts in the letterbox is
dropped, a drag that leaves the canvas is clamped. Down is a mouse down straight away (speed matters),
release sends the up and a leave so nothing stays highlighted.

## Rumble

The controller rumbles with the sounds of the moments that matter (`RumbleForSound` in
`game/WinFishApp.cpp`): an alien arriving (strong, every alien kind has its own sound) and the warning
before it, an alien going down or a bomb going off, a light tick for each shot that hits and for a tap
on the glass, a fish dying, a pet hatching. The sound manager tells the game about every sound that
starts (`SoundManager::mPlayHook`), so the game code itself is untouched. HD rumble at 160/320 Hz; a
weaker rumble doesn't cut a stronger one short; it stops on focus loss and on exit. Strengths are a first
guess until felt on hardware.

## Text

The Switch keyboard opens when a name field gets the focus, starting from the current text and with the
field's length limit. Tapping a field that already has the focus doesn't reopen it yet.

## Steps

1. Done: the cursor, touch, B, +, −, focus pause, a `pad` step for test plans (`port/TestPlan.h`), the
   framework fixes (mouse up forgot the pointer, frames only presented when the game redrew) and the food
   repeat zone on the wide tank. A game controller on the Mac drives the same cursor.
2. Done: D-pad to buttons, slower cursor over clickable things, double click, swkbd with the current text
   and length.
3. Done: USB mouse (after insaniquarium_nx's sample handling), rumble.
4. Waiting for hardware, because they need tuning on a real controller: gyro aiming (insaniquarium_nx had to
   flip axis signs per controller type), the rumble strengths, single Joy-Con held sideways, USB keyboard,
   reopening the keyboard when a focused name field is tapped.

Checked on the Mac (test plans and a controller-free run of the same code) and booted in Ryubing; how the
curve feels, touch, focus loss and swkbd still need a real Switch.

Not planned: magnetism toward coins or aliens (changes knockback), turbo or sweep collecting (would give
the machine gun early); if ever, opt-in.

## insaniquarium_nx

Taken or to take: B as right click, hover from the cursor position, swkbd, cursor on by default when
docked, their USB mouse sample handling, and later their gyro handle selection (proven on hardware). Done differently: their stick
is linear without a deadzone and per frame, there is no precision mode, every touch finger is a click,
the letterbox is fixed at 4:3, and X/Y send Escape and F1, which the original game ignores.
