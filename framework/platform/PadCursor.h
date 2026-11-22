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
	bool mJumpFish = false;		// X: the hungriest fish
	bool mJumpMoney = false;	// Y: the money that will be gone first
	bool mFast = false;			// R: fast-forward while held
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

// Rumble on the controller in use, strengths 0..1 for the low and high motors. On the Switch a weaker
// rumble doesn't cut a stronger one short. Implemented per platform (Input.cpp).
void RumblePad(float theLow, float theHigh, int theMs);

// Quick aim (X and Y): the game's theIndex'th target of theKind (0 the hungriest fish, 1 money; the index
// wraps), in widget coordinates. 1 found, 0 none in the tank, -1 not now (no board, a dialog, paused).
// theClickable says whether a click there does what the button means (it could land on a pet, or buzz when
// the food can't be paid for on a repeat). Set by the game (port/PadTargets.cpp).
extern int (*gPadTargetHook)(SexyAppBase* theApp, int theKind, int theIndex, bool theRepeat, int& theX, int& theY,
	bool& theClickable);

// The controller as a test plan sets it, used instead of the real one while input is scripted.
extern PadInput gScriptedPad;
}
