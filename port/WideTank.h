#pragma once

namespace Sexy
{
    class Graphics;
    class Image;
    class MemoryImage;
    class SexyAppBase;
}

// Art for the wide tank from the player's own backdrops: forgets what was built (the tank width changed),
// each tank's is built when it is first asked for (main thread, Board::Update or Draw).
void BuildWideBackdrops(Sexy::SexyAppBase* app);

// The backdrop for tank background 1..6 at full tank width, or null when the wide tank is off.
Sexy::Image* WideBackdrop(int theBackgroundId);
// The front floor strip (tank mask) to go with it, drawn at (TankLeft(), 365).
Sexy::Image* WideTankMask(int theBackgroundId);
// A 4:3 screen's backdrop across the whole canvas (theImageId's original in the middle, the pack
// widescreen/<thePack> around it), or null when the canvas is 4:3. Draw it centred on the screen.
Sexy::Image* WideScreenBackdrop(Sexy::SexyAppBase* app, int theImageId, const char* thePack);
// Frees one (the title screen's, once it is gone for good).
void ForgetWideScreenBackdrop(const char* thePack);
