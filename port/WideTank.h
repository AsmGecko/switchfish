#pragma once

namespace Sexy
{
    class Graphics;
    class Image;
    class MemoryImage;
    class SexyAppBase;
}

// Art for the wide tank, built at load time from the player's own backdrops.
void BuildWideBackdrops(Sexy::SexyAppBase* app);

// The backdrop for tank background 1..6 at full tank width, or null when the wide tank is off.
Sexy::Image* WideBackdrop(int theBackgroundId);
// The front floor strip (tank mask) to go with it, drawn at (TankLeft(), 365).
Sexy::Image* WideTankMask(int theBackgroundId);
