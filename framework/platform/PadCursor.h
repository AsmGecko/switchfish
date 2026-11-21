#pragma once

// switchfish: the controller cursor, shared by the Switch and the host (docs/CONTROLS.md).

namespace Sexy
{
class SexyAppBase;

// One frame of controller state. Sticks are -1..1 with y up.
struct PadInput
{
	float mLeftX = 0, mLeftY = 0;
	float mRightX = 0, mRightY = 0;
	bool mClick = false;
	bool mRightClick = false;
	bool mPrecision = false;
	bool mMenu = false;
	bool mPause = false;
	bool mUp = false, mDown = false, mLeft = false, mRight = false;
	// a USB mouse moves the same cursor, in design units this frame
	float mMouseX = 0, mMouseY = 0;
	// another pointer (a finger) owns the mouse: no motion or clicks, held buttons wait for a release
	bool mPointerElsewhere = false;
};

// Platforms read their controller into a PadInput once per frame and pass it here.
void UpdatePadCursor(SexyAppBase* theApp, const PadInput& theInput);
// Touch or a mouse took over at (theX, theY), in widget coordinates; the stick carries on from there.
void HidePadCursor(SexyAppBase* theApp, int theX, int theY);
// Clamps a point to what can be clicked; false if it was outside (the letterbox).
bool ClampPointer(SexyAppBase* theApp, int& theX, int& theY);
// Where to draw the cursor; false while it is hidden.
bool GetPadCursor(int& theX, int& theY);

// The controller as a test plan sets it, used instead of the real one while input is scripted.
extern PadInput gScriptedPad;
}
