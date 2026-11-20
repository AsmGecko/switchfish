#pragma once

namespace Sexy
{
    class SexyAppBase;
}

// Plays a test plan: input and screenshots at fixed logic ticks, in the game's 640x480 design coordinates.
//
//   # comment
//   <tick> click <x> <y>        left click (right click: rclick)
//   <tick> move <x> <y>
//   <tick> key <name>           escape, return, space, or a single character
//   <tick> shot <name>          writes <outdir>/<name>.png from the canvas
//   <tick> quit
//
// Ticks count from the first logic update (28 ms each). A line starting with + is relative to the previous one.
// Quits after timeoutSeconds no matter what, so a stuck run never stays open.
bool LoadTestPlan(Sexy::SexyAppBase* app, const char* planPath, const char* outDir, int timeoutSeconds);
